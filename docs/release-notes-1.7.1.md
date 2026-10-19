## NUchat 1.7.1

Bug-fix release on top of [1.7.0](https://github.com/lord3nd3r/NUchat/releases/tag/v1.7.0).

### Fixed
- **High CPU usage.** A channel with an unseen highlight pulsed its name in the sidebar with an animation that looped forever, which made the whole window redraw every frame. Highlighted channels are now shown in red and bold with no animation.
- **Busy channels kept a 50 ms timer running.** Every incoming message restarted the scroll-to-bottom settle timer. A live single-line append now scrolls once; the settle timer only runs after a full channel reload.
- **Default scrollback lowered from 10000 to 2000 lines** to keep the rich-text chat view and channel switches fast. The value can still be raised under Preferences.
