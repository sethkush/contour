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
#include <terminal/DECTextLocator.h>

#include <crispy/assert.h>
#include <crispy/unreachable.h>

using std::tuple;

namespace terminal
{

namespace
{
    enum class Event
    {
        LocatorUnavailable = 0,
        Request = 1, // reply-event to a received a DECRQLP
        LeftButtonDown = 2,
        LeftButtonUp = 3,
        MiddleButtonDown = 4,
        MiddleButtonUp = 5,
        RightButtonDown = 6,
        RightButtonUp = 7,
        WheelDown = 8, // M4 down
        WheelUp = 9,   // M4 up
        LocatorOutsideFilterRect = 10,
    };

    Event makeEvent(MouseButton button, bool pressed) noexcept
    {
        switch (button)
        {
        case MouseButton::None: return Event::LocatorUnavailable;
        case MouseButton::Left: return pressed ? Event::LeftButtonDown : Event::LeftButtonUp;
        case MouseButton::Middle: return pressed ? Event::MiddleButtonDown : Event::MiddleButtonUp;
        case MouseButton::Right: return pressed ? Event::RightButtonDown : Event::RightButtonUp;
        case MouseButton::WheelUp: return Event::WheelUp;
        case MouseButton::WheelDown: return Event::WheelDown;
        }

        Require(false && "Unhandled mouse button");
        crispy::unreachable();
    }

    enum class Page
    {
        One = 1,
    };

    std::string createReport(Event event, MouseButton button, LineOffset row, ColumnOffset column, Page page)
    {
        if (event == Event::LocatorUnavailable)
            return "\033[0&m";

        return fmt::format("\033[{};{};{};{};{}&w",
                           static_cast<unsigned>(event),
                           static_cast<unsigned>(button),
                           row.value,
                           column.value,
                           static_cast<unsigned>(page));
    }

} // namespace

void DECTextLocator::reset()
{
    // TODO(pr)
}

void DECTextLocator::selectLocatorEvents(DECLocatorEvent event, bool enabled) noexcept
{
    if (enabled)
        _selectedLocatorEvents |= static_cast<uint32_t>(event);
    else
        _selectedLocatorEvents &= ~static_cast<uint32_t>(event);
}

bool DECTextLocator::reportButtonUpEvents() const noexcept
{
    return _selectedLocatorEvents & static_cast<uint32_t>(DECLocatorEvent::ButtonUp);
}

bool DECTextLocator::reportButtonDownEvents() const noexcept
{
    return _selectedLocatorEvents & static_cast<uint32_t>(DECLocatorEvent::ButtonDown);
}

bool DECTextLocator::reportEventExplicitOnly() const noexcept
{
    return _selectedLocatorEvents == 0;
}

void DECTextLocator::disableLocatorReporting() noexcept
{
    _reportingMode = DECLocatorReportingMode::Disabled;
}

void DECTextLocator::enableLocatorReporting(CoordinateUnits units) noexcept
{
    _reportingMode = DECLocatorReportingMode::Enabled;
    _units = units;
}

void DECTextLocator::enableLocatorReportingOnce(CoordinateUnits units) noexcept
{
    _reportingMode = DECLocatorReportingMode::EnabledOnce;
    _units = units;
}

void DECTextLocator::enableFilterRectangle(DECLocatorRectangle rect) noexcept
{
    /*
        Defines the coordinates of a filter rectangle and activates
        it.

        - Anytime the locator is detected outside of the filter
          rectangle, an outside rectangle event is generated and the
          rectangle is disabled.
        - Filter rectangles are always treated as "one-shot" events.
        - Any parameters that are omitted default to the current locator position.
        - If all parameters are omitted, any locator motion will be reported.
        - DECELR always cancels any previous rectangle definition.
     */
    _reportingMode = DECLocatorReportingMode::FilterRectangular;
    _filterRectangle = { rect };
}

void DECTextLocator::disableFilterRectangle() noexcept
{
    _reportingMode = DECLocatorReportingMode::Disabled;
    // Leaving _filterRectangle undefined.
}

bool DECTextLocator::filterRectangleEnabled() const noexcept
{
    return _reportingMode == DECLocatorReportingMode::FilterRectangular;
}

void DECTextLocator::requestLocatorPosition() noexcept
{
    // TODO(pr)
}

std::string_view DECTextLocator::peekLocatorReply() const noexcept
{
    return std::string_view(_replyBuffer[_replyBackBufferIndex].data(),
                            _replyBuffer[_replyBackBufferIndex].size());
}

std::string_view DECTextLocator::fetchReplyAndClear() noexcept
{
    auto const resultIndex = _replyBackBufferIndex;
    _replyBackBufferIndex = (_replyBackBufferIndex + 1) % 2;
    return std::string_view(_replyBuffer[resultIndex].data(), _replyBuffer[resultIndex].size());
}

// void DECTextLocator::report(MouseButton button, bool buttonPressed)
// {
//     // Submits a single DECLRP locator report.
//     // <- CSI Pe ; Pb ; Pr ; Pc ; Pp &  w
//     // Parameters are [event;button;row;column;page].
//
//     if (_reportingMode == DECLocatorReportingMode::Disabled)
//     {
//         reply("\033[0&m"); // 0 = locator unavailable - no other parameters sent
//         return;
//     }
//
//     auto const Pe = [&]() {
//         // TODO(pr) if current mouse position is outside region, then return 0.
//         auto const shift = buttonPressed ? 0 : 1;
//         switch (button)
//         {
//         case MouseButton::None: return 0; // no button pressed
//         case MouseButton::Left: return 2 + shift;
//         case MouseButton::Middle: return 4 + shift;
//         case MouseButton::Right: return 6 + shift;
//         case MouseButton::WheelUp: return 8;   // M4-up as wheel-up?
//         case MouseButton::WheelDown: return 9; // M$-down as wheel-down?
//         }
//         return 0; // TODO(pr)
//     }();
//
//     auto const Pb = [&]() {
//         uint32_t mask = 0;
//         if (unsigned(_currentlyPressedMouseButtons) & unsigned(MouseButton::Right))
//             mask |= 1;
//         if (unsigned(_currentlyPressedMouseButtons) & unsigned(MouseButton::Middle))
//             mask |= 2;
//         if (unsigned(_currentlyPressedMouseButtons) & unsigned(MouseButton::Left))
//             mask |= 4;
//         // TODO(pr) Is M4 (bitmask 8) of interest for us?
//         // And if that's Wheel, we'd also need M5 and bitmask 18
//
//         return mask;
//     }();
//
//     auto const [Pr, Pc] = [&]() -> std::tuple<int, int> {
//         // report in pixel or in cell units
//         switch (_units)
//         {
//         case CoordinateUnits::Cells: return { _cellPosition.line.value, _cellPosition.column.value };
//         case CoordinateUnits::Pixels: return { _pixelPosition.y.value, _pixelPosition.x.value };
//         }
//         Require(false && "Internal Bug! CoordinateUnits is missing a case here.");
//     }();
//
//     auto const Pp = 1; // We only have one page.
//
//     reply("\033[{};{};{};{};{}&w", Pe, Pb, Pr, Pc, Pp);
// }

void DECTextLocator::updateMouseMove(CellLocation cellPosition, MousePixelPosition pixelPosition)
{
    _cellPosition = cellPosition;
    _pixelPosition = pixelPosition;
}

void DECTextLocator::updateMousePress(MouseButton button, bool buttonPressed)
{
    if (buttonPressed)
        _currentlyPressedMouseButtons =
            MouseButton(unsigned(_currentlyPressedMouseButtons) | unsigned(button));
    else
        _currentlyPressedMouseButtons =
            MouseButton(unsigned(_currentlyPressedMouseButtons) & ~unsigned(button));

    auto const event = makeEvent(button, buttonPressed);

    auto const [row, column] = [this]() -> tuple<LineOffset, ColumnOffset> {
        switch (_units)
        {
        case CoordinateUnits::Cells:
            // cell coordinates
            return tuple { _cellPosition.line, _cellPosition.column };
        case CoordinateUnits::Pixels:
            // pixel coordinates
            return tuple { LineOffset(_pixelPosition.y.value), ColumnOffset(_pixelPosition.x.value) };
        }
        crispy::unreachable();
    }();

    reply(createReport(event, button, row, column, Page::One));
}

void DECTextLocator::update(MouseButton button,
                            bool buttonPressed,
                            CellLocation cellPosition,
                            MousePixelPosition pixelPosition)
{
    switch (_reportingMode)
    {
    case DECLocatorReportingMode::Disabled:
        // nothing to be done here :-)
        break;
    case DECLocatorReportingMode::EnabledOnce: // DECELR (oneshot)
        // TODO(pr)
        _reportingMode = DECLocatorReportingMode::Disabled;
        break;
    case DECLocatorReportingMode::Enabled:           // DECELR | TODO(pr)
    case DECLocatorReportingMode::FilterRectangular: // DECEFR | TODO(pr)
        break;
    }
}

} // namespace terminal
