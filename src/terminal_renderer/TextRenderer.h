/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019-2020 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <terminal_renderer/Atlas.h>
#include <terminal_renderer/RenderTarget.h>

#include <terminal/Color.h>
#include <terminal/Screen.h>

#include <text_shaper/font.h>
#include <text_shaper/shaper.h>

#include <crispy/FNV.h>
#include <crispy/point.h>
#include <crispy/size.h>
#include <crispy/span.h>

#include <unicode/run_segmenter.h>

#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace terminal::renderer
{
    using GlyphId = text::glyph_key;

    struct CacheKey {
        std::u32string_view text;
        CharacterStyleMask styles;

        bool operator==(CacheKey const& _rhs) const noexcept
        {
            return text == _rhs.text && styles == _rhs.styles;
        }

        bool operator!=(CacheKey const& _rhs) const noexcept
        {
            return !(*this == _rhs);
        }

        bool operator<(CacheKey const& _rhs) const noexcept
        {
            if (text < _rhs.text)
                return true;

            if (static_cast<unsigned>(styles) < static_cast<unsigned>(_rhs.styles))
                return true;

            return false;
        }
    };

    enum class TextStyle {
        Invalid     = 0x00,
        Regular     = 0x10,
        Bold        = 0x11,
        Italic      = 0x12,
        BoldItalic  = 0x13,
    };

    constexpr TextStyle operator|(TextStyle a, TextStyle b) noexcept
    {
        return static_cast<TextStyle>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }

    constexpr bool operator<(TextStyle a, TextStyle b) noexcept
    {
        return static_cast<unsigned>(a) < static_cast<unsigned>(b);
    }

    struct TextCacheKey {
        std::u32string_view text;
        TextStyle style; // TODO: use font_key instead, and kill TextStyle

        constexpr bool operator<(TextCacheKey const& _rhs) const noexcept
        {
            if (text < _rhs.text)
                return true;

            return text < _rhs.text || style < _rhs.style;
        }

        constexpr bool operator==(TextCacheKey const& _rhs) const noexcept
        {
            return text == _rhs.text && style == _rhs.style;
        }

        constexpr bool operator!=(TextCacheKey const& _rhs) const noexcept
        {
            return !(*this == _rhs);
        }
    };
}

namespace std
{
    template <>
    struct hash<terminal::renderer::CacheKey> {
        size_t operator()(terminal::renderer::CacheKey const& _key) const noexcept
        {
            auto fnv = crispy::FNV<char32_t>{};
            return static_cast<size_t>(fnv(fnv(_key.text.data(), _key.text.size()), static_cast<char32_t>(_key.styles)));
        }
    };

    template <>
    struct hash<terminal::renderer::TextCacheKey> {
        size_t operator()(terminal::renderer::TextCacheKey const& _key) const noexcept
        {
            auto fnv = crispy::FNV<char32_t>{};
            return static_cast<size_t>(fnv(
                fnv.basis(),
                _key.text,
                static_cast<char32_t>(_key.style)
            ));
        }
    };
}

namespace terminal::renderer {

struct GridMetrics;

struct FontDescriptions {
    text::font_size size;
    text::font_description regular;
    text::font_description bold;
    text::font_description italic;
    text::font_description boldItalic;
    text::font_description emoji;
    text::render_mode renderMode;
};

inline bool operator==(FontDescriptions const& a, FontDescriptions const& b) noexcept
{
    return a.size.pt == b.size.pt
        && a.regular == b.regular
        && a.bold == b.bold
        && a.italic == b.italic
        && a.boldItalic == b.boldItalic
        && a.emoji == b.emoji
        && a.renderMode == b.renderMode;
}

inline bool operator!=(FontDescriptions const& a, FontDescriptions const& b) noexcept
{
    return !(a == b);
}

struct FontKeys {
    text::font_key regular;
    text::font_key bold;
    text::font_key italic;
    text::font_key boldItalic;
    text::font_key emoji;
};

// {{{ TextShaper
/// API to perform text shaping and glyph rasterization on terminal screen.
class TextShaper
{
public:
    using RenderGlyphs = std::function<void(crispy::Point,
                                            crispy::span<text::glyph_position const>,
                                            RGBColor)>;

    virtual ~TextShaper() = default;

    virtual void clearCache() = 0;

    /// Puts a sequence of codepoints that belong to the same grid cell at @p _pos
    /// at the end of the currently filled line.
    ///
    virtual void appendCell(crispy::span<char32_t const> _codepoints,
                            TextStyle _style,
                            RGBColor _color) = 0;

    /// Marks the end of a rendered line.
    virtual void endLine() = 0;

    /// Marks the end of a rendered frame.
    virtual void endFrame() = 0;
};

// Fully featured Text shaping pipeline.
class StandardTextShaper : public TextShaper
{
public:
    StandardTextShaper(GridMetrics const& _gridMetrics,
                       text::shaper& _textShaper,
                       FontDescriptions const& _fontDescriptions,
                       FontKeys const& _fonts,
                       RenderGlyphs _renderGlyphs);

    void clearCache() override;

    /// Puts a sequence of codepoints that belong to the same grid cell at @p _pos
    /// at the end of the currently filled line.
    ///
    void appendCell(crispy::span<char32_t const> _codepoints,
                    TextStyle _style,
                    RGBColor _color) override;

    /// Marks the end of a rendered line.
    void endLine() override;

    /// Marks the end of a rendered frame.
    void endFrame() override;

private:
    // helper functions
    //
    void reset(Coordinate _pos, TextStyle _style, RGBColor _color);
    void extend(crispy::span<char32_t const> _codepoints);
    void flushPendingSegments();
    text::shape_result const& cachedGlyphPositions();
    text::shape_result requestGlyphPositions();
    text::shape_result shapeRun(unicode::run_segmenter::range const& _run);

    // fonts, text shaper, and grid metrics
    //
    GridMetrics const& gridMetrics_;
    FontDescriptions const& fontDescriptions_;
    FontKeys const& fonts_;
    text::shaper& textShaper_;
    RenderGlyphs renderGlyphs_;

    // render states
    //
    enum class State { Empty, Filling };
    State state_ = State::Empty;
    int currentLine_ = 1;
    int startColumn_ = 1;
    TextStyle style_ = TextStyle::Invalid;
    RGBColor color_{};

    std::vector<char32_t> codepoints_;
    std::vector<int> clusters_;
    int clusterOffset_ = 0;

    // text shaping cache
    //
    std::list<std::u32string> cacheKeyStorage_;
    std::unordered_map<TextCacheKey, text::shape_result> cache_;

    // output fields
    //
    std::vector<text::shape_result> shapedLines_;
};

// Text rendering pipeline optimized for performance with simple feature set.
class SimpleTextShaper : public TextShaper
{
    // TODO: only uses trivial freetype like calls (plus caching, no harfbuzz)
};
// }}}

/// Text Rendering Pipeline
class TextRenderer : public Renderable {
  public:
    TextRenderer(GridMetrics const& _gridMetrics,
                 text::shaper& _textShaper,
                 FontDescriptions& _fontDescriptions,
                 FontKeys const& _fontKeys);

    void setRenderTarget(RenderTarget& _renderTarget) override;
    void clearCache() override;

    void updateFontMetrics();

    void setPressure(bool _pressure) noexcept { pressure_ = _pressure; }

    void schedule(Coordinate const& _pos, Cell const& _cell, RGBColor const& _color);
    void finish();

    void debugCache(std::ostream& _textOutput) const;

  private:
    void renderRun(crispy::Point _startPos,
                   crispy::span<text::glyph_position const> _glyphPositions,
                   RGBColor _color);

    /// Renders an arbitrary texture.
    void renderTexture(crispy::Point const& _pos,
                       RGBAColor const& _color,
                       atlas::TextureInfo const& _textureInfo);

    // rendering
    //
    struct GlyphMetrics {
        crispy::Size bitmapSize;    // glyph size in pixels
        crispy::Point bearing;      // offset baseline and left to top and left of the glyph's bitmap
    };
    friend struct fmt::formatter<GlyphMetrics>;

    using TextureAtlas = atlas::MetadataTextureAtlas<text::glyph_key, GlyphMetrics>;
    using DataRef = TextureAtlas::DataRef;

    std::optional<DataRef> getTextureInfo(GlyphId const& _id);

    void renderTexture(crispy::Point const& _pos,
                       RGBAColor const& _color,
                       atlas::TextureInfo const& _textureInfo,
                       GlyphMetrics const& _glyphMetrics,
                       text::glyph_position const& _gpos);

    TextureAtlas& atlasForFont(text::font_key _font);

    // general properties
    //
    GridMetrics const& gridMetrics_;
    FontDescriptions& fontDescriptions_;
    FontKeys const& fonts_;

    int row_ = 1;

    // performance optimizations
    //
    bool pressure_ = false;

    std::unordered_map<text::glyph_key, text::bitmap_format> glyphToTextureMapping_;

    TextureAtlas* atlasForBitmapFormat(text::bitmap_format _format)
    {
        switch (_format)
        {
            case text::bitmap_format::alpha_mask: return monochromeAtlas_.get();
            case text::bitmap_format::rgba: return colorAtlas_.get();
            case text::bitmap_format::rgb: return lcdAtlas_.get();
            default: return nullptr; // Should NEVER EVER happen.
        }
    }

    // target surface rendering
    //
    text::shaper& textShaper_;
    std::unique_ptr<TextureAtlas> monochromeAtlas_;
    std::unique_ptr<TextureAtlas> colorAtlas_;
    std::unique_ptr<TextureAtlas> lcdAtlas_;

    std::unique_ptr<TextShaper> textRenderingEngine_;
};

} // end namespace

namespace fmt { // {{{
    template <>
    struct formatter<terminal::renderer::FontDescriptions> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(terminal::renderer::FontDescriptions const& fd, FormatContext& ctx)
        {
            return format_to(
                ctx.out(),
                "({}, {}, {}, {}, {}, {})",
                fd.size,
                fd.regular,
                fd.bold,
                fd.italic,
                fd.boldItalic,
                fd.emoji,
                fd.renderMode);
        }
    };
} // }}}
