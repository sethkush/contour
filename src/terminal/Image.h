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

#include <terminal/Color.h>
#include <terminal/primitives.h>

#include <crispy/StrongHash.h>
#include <crispy/StrongLRUCache.h>

#include <fmt/format.h>

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace terminal
{

// XXX DRAFT
// Do we want to keep an Image that keeps the whole image together, and then cut it into grid-cell
// slices (requires reference counting on main image)?
// Or do we want to deal with Image slices right away and just keep those?
// The latter doesn't require reference counting.

enum class ImageFormat
{
    RGB,
    RGBA,
    PNG,
};

// clang-format off
namespace detail { struct ImageId {}; }
using ImageId = crispy::boxed<uint32_t, detail::ImageId>; // unique numerical image identifier
// clang-format on

struct ImageStats
{
    uint32_t instances = 0;
    uint32_t rasterized = 0;
    uint32_t fragments = 0;

    static ImageStats& get();
};

/**
 * Represents an image that can be displayed in the terminal by being placed into the grid cells
 */
class Image: public std::enable_shared_from_this<Image>
{
  public:
    using Data = std::vector<uint8_t>; // raw RGBA data
    using OnImageRemove = std::function<void(Image const*)>;
    /// Constructs an RGBA image.
    ///
    /// @param _data      RGBA buffer data
    /// @param _pixelSize image dimensionss in pixels
    Image(ImageId _id, ImageFormat _format, Data _data, ImageSize _pixelSize, OnImageRemove remover):
        id_ { _id },
        format_ { _format },
        data_ { move(_data) },
        size_ { _pixelSize },
        onImageRemove_ { std::move(remover) }
    {
        ++ImageStats::get().instances;
    }

    ~Image();

    Image(Image const&) = delete;
    Image& operator=(Image const&) = delete;
    Image(Image&&) = default;
    Image& operator=(Image&&) = default;

    constexpr ImageId id() const noexcept { return id_; }
    constexpr ImageFormat format() const noexcept { return format_; }
    Data const& data() const noexcept { return data_; }
    constexpr ImageSize size() const noexcept { return size_; }
    constexpr Width width() const noexcept { return size_.width; }
    constexpr Height height() const noexcept { return size_.height; }

  private:
    ImageId id_;
    ImageFormat format_;
    Data data_;
    ImageSize size_;
    OnImageRemove onImageRemove_;
};

/// Image resize hints are used to properly fit/fill the area to place the image onto.
enum class ImageResize
{
    NoResize,
    ResizeToFit, // default
    ResizeToFill,
    StretchToFill,
};

/// Image alignment policy are used to properly align the image to a given spot when not fully
/// filling the area this image as to be placed to.
enum class ImageAlignment
{
    TopStart,
    TopCenter,
    TopEnd,
    MiddleStart,
    MiddleCenter, // default
    MiddleEnd,
    BottomStart,
    BottomCenter,
    BottomEnd
};

/**
 * RasterizedImage wraps an Image into a fixed-size grid with some additional graphical properties for
 * rasterization.
 */
class RasterizedImage: public std::enable_shared_from_this<RasterizedImage>
{
  public:
    RasterizedImage(std::shared_ptr<Image const> _image,
                    ImageAlignment _alignmentPolicy,
                    ImageResize _resizePolicy,
                    RGBAColor _defaultColor,
                    GridSize _cellSpan,
                    ImageSize _cellSize):
        image_ { std::move(_image) },
        alignmentPolicy_ { _alignmentPolicy },
        resizePolicy_ { _resizePolicy },
        defaultColor_ { _defaultColor },
        cellSpan_ { _cellSpan },
        cellSize_ { _cellSize }
    {
        ++ImageStats::get().rasterized;
    }

    ~RasterizedImage();

    RasterizedImage(RasterizedImage const&) = delete;
    RasterizedImage(RasterizedImage&&) = delete;
    RasterizedImage& operator=(RasterizedImage const&) = delete;
    RasterizedImage& operator=(RasterizedImage&&) = delete;

    bool valid() const noexcept { return !!image_; }

    std::shared_ptr<Image const> const& imagePointer() const noexcept { return image_; }
    Image const& image() const noexcept { return *image_; }
    ImageAlignment alignmentPolicy() const noexcept { return alignmentPolicy_; }
    ImageResize resizePolicy() const noexcept { return resizePolicy_; }
    RGBAColor defaultColor() const noexcept { return defaultColor_; }
    GridSize cellSpan() const noexcept { return cellSpan_; }
    ImageSize cellSize() const noexcept { return cellSize_; }

    /// @returns an RGBA buffer for a grid cell at given coordinate @p _pos of the rasterized image.
    Image::Data fragment(CellLocation _pos) const;

  private:
    std::shared_ptr<Image const> const image_; //!< Reference to the Image to be rasterized.
    ImageAlignment const alignmentPolicy_;     //!< Alignment policy of the image inside the raster size.
    ImageResize const resizePolicy_;           //!< Image resize policy
    RGBAColor const defaultColor_;             //!< Default color to be applied at corners when needed.
    GridSize const cellSpan_;                  //!< Number of grid cells to span the pixel image onto.
    ImageSize const cellSize_; //!< number of pixels in X and Y dimension one grid cell has to fill.
};

/// An ImageFragment holds a graphical image that ocupies one full grid cell.
class ImageFragment
{
  public:
    ImageFragment() = delete;

    /// @param _image  the Image this fragment is being cut off from
    /// @param _offset 0-based grid-offset into the rasterized image
    ImageFragment(std::shared_ptr<RasterizedImage const> _image, CellLocation _offset):
        rasterizedImage_ { std::move(_image) }, offset_ { _offset }
    {
        ++ImageStats::get().fragments;
    }

    ImageFragment(ImageFragment const&) = delete;
    ImageFragment& operator=(ImageFragment const&) = delete;

    ImageFragment(ImageFragment&&) noexcept = default;
    ImageFragment& operator=(ImageFragment&&) noexcept = default;

    ~ImageFragment();

    [[nodiscard]] RasterizedImage const& rasterizedImage() const noexcept { return *rasterizedImage_; }

    /// @returns offset of this image fragment in pixels into the underlying image.
    CellLocation offset() const noexcept { return offset_; }

    /// Extracts the data from the image that is to be rendered.
    [[nodiscard]] Image::Data data() const { return rasterizedImage_->fragment(offset_); }

  private:
    std::shared_ptr<RasterizedImage const> rasterizedImage_;
    CellLocation offset_;
};

namespace detail
{
    struct ImageFragmentId;
}
using ImageFragmentId = crispy::boxed<uint16_t, detail::ImageFragmentId>;

inline bool operator==(ImageFragment const& a, ImageFragment const& b) noexcept
{
    return a.rasterizedImage().image().id() == b.rasterizedImage().image().id() && a.offset() == b.offset();
}

inline bool operator!=(ImageFragment const& a, ImageFragment const& b) noexcept
{
    return !(a == b);
}

inline bool operator<(ImageFragment const& a, ImageFragment const& b) noexcept
{
    return (a.rasterizedImage().image().id() < b.rasterizedImage().image().id())
           || (a.rasterizedImage().image().id() == b.rasterizedImage().image().id()
               && a.offset() < b.offset());
}

/// Highlevel Image Storage Pool.
///
/// Stores RGBA images in host memory, also taking care of eviction.
class ImagePool
{
  public:
    using OnImageRemove = std::function<void(Image const*)>;

    ImagePool(
        OnImageRemove _onImageRemove = [](auto) {}, ImageId _nextImageId = ImageId(1));

    /// Creates an RGBA image of given size in pixels.
    std::shared_ptr<Image const> create(ImageFormat _format, ImageSize _pixelSize, Image::Data&& _data);

    /// Rasterizes an Image.
    std::shared_ptr<RasterizedImage> rasterize(std::shared_ptr<Image const> _image,
                                               ImageAlignment _alignmentPolicy,
                                               ImageResize _resizePolicy,
                                               RGBAColor _defaultColor,
                                               GridSize _cellSpan,
                                               ImageSize _cellSize);

    // named image access
    //
    void link(std::string _name, std::shared_ptr<Image const> _imageRef);
    [[nodiscard]] std::shared_ptr<Image const> findImageByName(std::string const& _name) const noexcept;
    void unlink(std::string const& _name);

    void inspect(std::ostream& os) const;

    void clear();

  private:
    void removeRasterizedImage(RasterizedImage* _image); //!< Removes a rasterized image from pool.

    using NameToImageIdCache = crispy::StrongLRUCache<std::string, std::shared_ptr<Image const>>;

    // data members
    //
    ImageId nextImageId_;                      //!< ID for next image to be put into the pool
    NameToImageIdCache imageNameToImageCache_; //!< keeps mapping from name to raw image
    OnImageRemove const onImageRemove_;        //!< Callback to be invoked when image gets removed from pool.
};

} // namespace terminal

namespace fmt // {{{
{

template <>
struct formatter<terminal::ImageFormat>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(terminal::ImageFormat value, FormatContext& ctx)
    {
        switch (value)
        {
            case terminal::ImageFormat::RGB: return format_to(ctx.out(), "RGB");
            case terminal::ImageFormat::RGBA: return format_to(ctx.out(), "RGBA");
            case terminal::ImageFormat::PNG: return format_to(ctx.out(), "PNG");
        }
        return format_to(ctx.out(), "{}", unsigned(value));
    }
};

template <>
struct formatter<terminal::ImageStats>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(terminal::ImageStats stats, FormatContext& ctx)
    {
        return format_to(ctx.out(),
                         "{} instances, {} raster, {} fragments",
                         stats.instances,
                         stats.rasterized,
                         stats.fragments);
    }
};

template <>
struct formatter<std::shared_ptr<terminal::Image const>>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(std::shared_ptr<terminal::Image const> const& _image, FormatContext& ctx)
    {
        if (!_image)
            return format_to(ctx.out(), "nullptr");
        terminal::Image const& image = *_image;
        return format_to(ctx.out(),
                         "Image<#{}, {}, size={}>",
                         image.weak_from_this().use_count(),
                         image.id(),
                         image.size());
    }
};

template <>
struct formatter<terminal::ImageResize>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const terminal::ImageResize _value, FormatContext& ctx)
    {
        switch (_value)
        {
            case terminal::ImageResize::NoResize: return format_to(ctx.out(), "NoResize");
            case terminal::ImageResize::ResizeToFit: return format_to(ctx.out(), "ResizeToFit");
            case terminal::ImageResize::ResizeToFill: return format_to(ctx.out(), "ResizeToFill");
            case terminal::ImageResize::StretchToFill: return format_to(ctx.out(), "StretchToFill");
        }
        return format_to(ctx.out(), "ResizePolicy({})", int(_value));
    }
};

template <>
struct formatter<terminal::ImageAlignment>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const terminal::ImageAlignment _value, FormatContext& ctx)
    {
        switch (_value)
        {
            case terminal::ImageAlignment::TopStart: return format_to(ctx.out(), "TopStart");
            case terminal::ImageAlignment::TopCenter: return format_to(ctx.out(), "TopCenter");
            case terminal::ImageAlignment::TopEnd: return format_to(ctx.out(), "TopEnd");
            case terminal::ImageAlignment::MiddleStart: return format_to(ctx.out(), "MiddleStart");
            case terminal::ImageAlignment::MiddleCenter: return format_to(ctx.out(), "MiddleCenter");
            case terminal::ImageAlignment::MiddleEnd: return format_to(ctx.out(), "MiddleEnd");
            case terminal::ImageAlignment::BottomStart: return format_to(ctx.out(), "BottomStart");
            case terminal::ImageAlignment::BottomCenter: return format_to(ctx.out(), "BottomCenter");
            case terminal::ImageAlignment::BottomEnd: return format_to(ctx.out(), "BottomEnd");
        }
        return format_to(ctx.out(), "ImageAlignment({})", int(_value));
    }
};

template <>
struct formatter<terminal::RasterizedImage>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const terminal::RasterizedImage& _image, FormatContext& ctx)
    {
        return format_to(ctx.out(),
                         "RasterizedImage<{}, {}, {}, {}, {}>",
                         _image.weak_from_this().use_count(),
                         _image.cellSpan(),
                         _image.resizePolicy(),
                         _image.alignmentPolicy(),
                         _image.image());
    }
};

template <>
struct formatter<terminal::ImageFragment>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const terminal::ImageFragment& _fragment, FormatContext& ctx)
    {
        return format_to(
            ctx.out(), "ImageFragment<offset={}, {}>", _fragment.offset(), _fragment.rasterizedImage());
    }
};
} // namespace fmt
