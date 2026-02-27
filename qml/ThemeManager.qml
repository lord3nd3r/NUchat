import QtQuick 2.15

QtObject {
    id: themeManager

    // ── Current theme index (persisted via Settings if desired) ──
    property int currentThemeIndex: 0

    // ── Derived current theme ──
    property var currentTheme: themes[currentThemeIndex] || themes[0]

    // ── Convenience accessors ──
    property string name:             currentTheme.name
    property color windowBg:          currentTheme.windowBg
    property color sidebarBg:         currentTheme.sidebarBg
    property color sidebarHeaderBg:   currentTheme.sidebarHeaderBg
    property color menuBarBg:         currentTheme.menuBarBg
    property color chatBg:            currentTheme.chatBg
    property color inputBg:           currentTheme.inputBg
    property color inputBorder:       currentTheme.inputBorder
    property color inputBorderFocus:  currentTheme.inputBorderFocus
    property color nickListBg:        currentTheme.nickListBg
    property color nickListHeaderBg:  currentTheme.nickListHeaderBg
    property color topicBg:           currentTheme.topicBg
    property color separator:         currentTheme.separator
    property color textPrimary:       currentTheme.textPrimary
    property color textSecondary:     currentTheme.textSecondary
    property color textMuted:         currentTheme.textMuted
    property color textInput:         currentTheme.textInput
    property color placeholder:       currentTheme.placeholder
    property color highlight:         currentTheme.highlight
    property color highlightText:     currentTheme.highlightText
    property color hoverBg:           currentTheme.hoverBg
    property color selectedBg:        currentTheme.selectedBg
    property color selectedText:      currentTheme.selectedText
    property color buttonBg:          currentTheme.buttonBg
    property color buttonPressed:     currentTheme.buttonPressed
    property color buttonDisabled:    currentTheme.buttonDisabled
    property color buttonText:        currentTheme.buttonText
    property color buttonTextDisabled:currentTheme.buttonTextDisabled
    property color dialogBg:          currentTheme.dialogBg
    property color dialogBorder:      currentTheme.dialogBorder
    property color dialogFieldBg:     currentTheme.dialogFieldBg
    property color dialogFieldBorder: currentTheme.dialogFieldBorder
    property color menuBg:            currentTheme.menuBg
    property color menuText:          currentTheme.menuText
    property color menuHighlight:     currentTheme.menuHighlight
    property color menuHighlightText: currentTheme.menuHighlightText
    // Nick colors by rank
    property color nickOwner:         currentTheme.nickOwner
    property color nickAdmin:         currentTheme.nickAdmin
    property color nickOp:            currentTheme.nickOp
    property color nickHalfOp:        currentTheme.nickHalfOp
    property color nickVoice:         currentTheme.nickVoice
    property color nickNormal:        currentTheme.nickNormal
    // Danger/error colors
    property color danger:            currentTheme.danger

    // ── Theme list ──
    property var themes: [
        // 0 - HexChat Dark (current default)
        {
            name: "HexChat Dark",
            windowBg: "#2b2b2b", sidebarBg: "#1e1e1e", sidebarHeaderBg: "#181818",
            menuBarBg: "#252526", chatBg: "#2b2b2b", inputBg: "#333",
            inputBorder: "#555", inputBorderFocus: "#569cd6",
            nickListBg: "#1e1e1e", nickListHeaderBg: "#181818",
            topicBg: "#252526", separator: "#404040",
            textPrimary: "#d4d4d4", textSecondary: "#ccc", textMuted: "#888",
            textInput: "#d4d4d4", placeholder: "#666",
            highlight: "#264f78", highlightText: "#fff",
            hoverBg: "#333", selectedBg: "#264f78", selectedText: "#fff",
            buttonBg: "#0e639c", buttonPressed: "#1177bb", buttonDisabled: "#444",
            buttonText: "#fff", buttonTextDisabled: "#777",
            dialogBg: "#2b2b2b", dialogBorder: "#555", dialogFieldBg: "#333", dialogFieldBorder: "#555",
            menuBg: "#2b2b2b", menuText: "#ddd", menuHighlight: "#264f78", menuHighlightText: "#fff",
            nickOwner: "#f44747", nickAdmin: "#ce9178", nickOp: "#4ec9b0",
            nickHalfOp: "#c586c0", nickVoice: "#dcdcaa", nickNormal: "#ccc",
            danger: "#f44747"
        },
        // 1 - Monokai
        {
            name: "Monokai",
            windowBg: "#272822", sidebarBg: "#1e1f1c", sidebarHeaderBg: "#1a1b18",
            menuBarBg: "#2d2e27", chatBg: "#272822", inputBg: "#3e3d32",
            inputBorder: "#555", inputBorderFocus: "#a6e22e",
            nickListBg: "#1e1f1c", nickListHeaderBg: "#1a1b18",
            topicBg: "#2d2e27", separator: "#49483e",
            textPrimary: "#f8f8f2", textSecondary: "#cfcfc2", textMuted: "#75715e",
            textInput: "#f8f8f2", placeholder: "#75715e",
            highlight: "#49483e", highlightText: "#f8f8f2",
            hoverBg: "#3e3d32", selectedBg: "#49483e", selectedText: "#f8f8f2",
            buttonBg: "#a6e22e", buttonPressed: "#b8f33c", buttonDisabled: "#49483e",
            buttonText: "#272822", buttonTextDisabled: "#75715e",
            dialogBg: "#272822", dialogBorder: "#555", dialogFieldBg: "#3e3d32", dialogFieldBorder: "#555",
            menuBg: "#272822", menuText: "#f8f8f2", menuHighlight: "#49483e", menuHighlightText: "#f8f8f2",
            nickOwner: "#f92672", nickAdmin: "#fd971f", nickOp: "#a6e22e",
            nickHalfOp: "#ae81ff", nickVoice: "#e6db74", nickNormal: "#f8f8f2",
            danger: "#f92672"
        },
        // 2 - Solarized Dark
        {
            name: "Solarized Dark",
            windowBg: "#002b36", sidebarBg: "#073642", sidebarHeaderBg: "#002b36",
            menuBarBg: "#073642", chatBg: "#002b36", inputBg: "#073642",
            inputBorder: "#586e75", inputBorderFocus: "#268bd2",
            nickListBg: "#073642", nickListHeaderBg: "#002b36",
            topicBg: "#073642", separator: "#586e75",
            textPrimary: "#839496", textSecondary: "#93a1a1", textMuted: "#586e75",
            textInput: "#93a1a1", placeholder: "#586e75",
            highlight: "#073642", highlightText: "#eee8d5",
            hoverBg: "#073642", selectedBg: "#274b56", selectedText: "#eee8d5",
            buttonBg: "#268bd2", buttonPressed: "#2aa0f0", buttonDisabled: "#073642",
            buttonText: "#fdf6e3", buttonTextDisabled: "#586e75",
            dialogBg: "#002b36", dialogBorder: "#586e75", dialogFieldBg: "#073642", dialogFieldBorder: "#586e75",
            menuBg: "#002b36", menuText: "#839496", menuHighlight: "#073642", menuHighlightText: "#eee8d5",
            nickOwner: "#dc322f", nickAdmin: "#cb4b16", nickOp: "#859900",
            nickHalfOp: "#6c71c4", nickVoice: "#b58900", nickNormal: "#839496",
            danger: "#dc322f"
        },
        // 3 - Solarized Light
        {
            name: "Solarized Light",
            windowBg: "#fdf6e3", sidebarBg: "#eee8d5", sidebarHeaderBg: "#e0dcc7",
            menuBarBg: "#eee8d5", chatBg: "#fdf6e3", inputBg: "#eee8d5",
            inputBorder: "#93a1a1", inputBorderFocus: "#268bd2",
            nickListBg: "#eee8d5", nickListHeaderBg: "#e0dcc7",
            topicBg: "#eee8d5", separator: "#93a1a1",
            textPrimary: "#657b83", textSecondary: "#586e75", textMuted: "#93a1a1",
            textInput: "#586e75", placeholder: "#93a1a1",
            highlight: "#eee8d5", highlightText: "#002b36",
            hoverBg: "#e0dcc7", selectedBg: "#268bd2", selectedText: "#fdf6e3",
            buttonBg: "#268bd2", buttonPressed: "#2aa0f0", buttonDisabled: "#eee8d5",
            buttonText: "#fdf6e3", buttonTextDisabled: "#93a1a1",
            dialogBg: "#fdf6e3", dialogBorder: "#93a1a1", dialogFieldBg: "#eee8d5", dialogFieldBorder: "#93a1a1",
            menuBg: "#fdf6e3", menuText: "#657b83", menuHighlight: "#eee8d5", menuHighlightText: "#002b36",
            nickOwner: "#dc322f", nickAdmin: "#cb4b16", nickOp: "#859900",
            nickHalfOp: "#6c71c4", nickVoice: "#b58900", nickNormal: "#657b83",
            danger: "#dc322f"
        },
        // 4 - Dracula
        {
            name: "Dracula",
            windowBg: "#282a36", sidebarBg: "#21222c", sidebarHeaderBg: "#191a21",
            menuBarBg: "#21222c", chatBg: "#282a36", inputBg: "#343746",
            inputBorder: "#6272a4", inputBorderFocus: "#bd93f9",
            nickListBg: "#21222c", nickListHeaderBg: "#191a21",
            topicBg: "#21222c", separator: "#44475a",
            textPrimary: "#f8f8f2", textSecondary: "#e0e0e0", textMuted: "#6272a4",
            textInput: "#f8f8f2", placeholder: "#6272a4",
            highlight: "#44475a", highlightText: "#f8f8f2",
            hoverBg: "#343746", selectedBg: "#44475a", selectedText: "#f8f8f2",
            buttonBg: "#bd93f9", buttonPressed: "#cfa9ff", buttonDisabled: "#44475a",
            buttonText: "#282a36", buttonTextDisabled: "#6272a4",
            dialogBg: "#282a36", dialogBorder: "#6272a4", dialogFieldBg: "#343746", dialogFieldBorder: "#6272a4",
            menuBg: "#282a36", menuText: "#f8f8f2", menuHighlight: "#44475a", menuHighlightText: "#f8f8f2",
            nickOwner: "#ff5555", nickAdmin: "#ffb86c", nickOp: "#50fa7b",
            nickHalfOp: "#bd93f9", nickVoice: "#f1fa8c", nickNormal: "#f8f8f2",
            danger: "#ff5555"
        },
        // 5 - Nord
        {
            name: "Nord",
            windowBg: "#2e3440", sidebarBg: "#3b4252", sidebarHeaderBg: "#2e3440",
            menuBarBg: "#3b4252", chatBg: "#2e3440", inputBg: "#3b4252",
            inputBorder: "#4c566a", inputBorderFocus: "#88c0d0",
            nickListBg: "#3b4252", nickListHeaderBg: "#2e3440",
            topicBg: "#3b4252", separator: "#4c566a",
            textPrimary: "#d8dee9", textSecondary: "#e5e9f0", textMuted: "#4c566a",
            textInput: "#eceff4", placeholder: "#4c566a",
            highlight: "#434c5e", highlightText: "#eceff4",
            hoverBg: "#434c5e", selectedBg: "#434c5e", selectedText: "#eceff4",
            buttonBg: "#5e81ac", buttonPressed: "#81a1c1", buttonDisabled: "#4c566a",
            buttonText: "#eceff4", buttonTextDisabled: "#4c566a",
            dialogBg: "#2e3440", dialogBorder: "#4c566a", dialogFieldBg: "#3b4252", dialogFieldBorder: "#4c566a",
            menuBg: "#2e3440", menuText: "#d8dee9", menuHighlight: "#434c5e", menuHighlightText: "#eceff4",
            nickOwner: "#bf616a", nickAdmin: "#d08770", nickOp: "#a3be8c",
            nickHalfOp: "#b48ead", nickVoice: "#ebcb8b", nickNormal: "#d8dee9",
            danger: "#bf616a"
        },
        // 6 - Gruvbox Dark
        {
            name: "Gruvbox Dark",
            windowBg: "#282828", sidebarBg: "#1d2021", sidebarHeaderBg: "#1d2021",
            menuBarBg: "#282828", chatBg: "#282828", inputBg: "#3c3836",
            inputBorder: "#504945", inputBorderFocus: "#fabd2f",
            nickListBg: "#1d2021", nickListHeaderBg: "#1d2021",
            topicBg: "#282828", separator: "#504945",
            textPrimary: "#ebdbb2", textSecondary: "#d5c4a1", textMuted: "#665c54",
            textInput: "#ebdbb2", placeholder: "#665c54",
            highlight: "#3c3836", highlightText: "#fbf1c7",
            hoverBg: "#3c3836", selectedBg: "#504945", selectedText: "#fbf1c7",
            buttonBg: "#98971a", buttonPressed: "#b8bb26", buttonDisabled: "#504945",
            buttonText: "#282828", buttonTextDisabled: "#665c54",
            dialogBg: "#282828", dialogBorder: "#504945", dialogFieldBg: "#3c3836", dialogFieldBorder: "#504945",
            menuBg: "#282828", menuText: "#ebdbb2", menuHighlight: "#504945", menuHighlightText: "#fbf1c7",
            nickOwner: "#cc241d", nickAdmin: "#d65d0e", nickOp: "#98971a",
            nickHalfOp: "#b16286", nickVoice: "#d79921", nickNormal: "#ebdbb2",
            danger: "#fb4934"
        },
        // 7 - Gruvbox Light
        {
            name: "Gruvbox Light",
            windowBg: "#fbf1c7", sidebarBg: "#ebdbb2", sidebarHeaderBg: "#d5c4a1",
            menuBarBg: "#ebdbb2", chatBg: "#fbf1c7", inputBg: "#ebdbb2",
            inputBorder: "#a89984", inputBorderFocus: "#d79921",
            nickListBg: "#ebdbb2", nickListHeaderBg: "#d5c4a1",
            topicBg: "#ebdbb2", separator: "#a89984",
            textPrimary: "#3c3836", textSecondary: "#504945", textMuted: "#928374",
            textInput: "#3c3836", placeholder: "#928374",
            highlight: "#d5c4a1", highlightText: "#282828",
            hoverBg: "#d5c4a1", selectedBg: "#d79921", selectedText: "#fbf1c7",
            buttonBg: "#79740e", buttonPressed: "#98971a", buttonDisabled: "#d5c4a1",
            buttonText: "#fbf1c7", buttonTextDisabled: "#928374",
            dialogBg: "#fbf1c7", dialogBorder: "#a89984", dialogFieldBg: "#ebdbb2", dialogFieldBorder: "#a89984",
            menuBg: "#fbf1c7", menuText: "#3c3836", menuHighlight: "#d5c4a1", menuHighlightText: "#282828",
            nickOwner: "#cc241d", nickAdmin: "#d65d0e", nickOp: "#79740e",
            nickHalfOp: "#8f3f71", nickVoice: "#b57614", nickNormal: "#3c3836",
            danger: "#cc241d"
        },
        // 8 - One Dark
        {
            name: "One Dark",
            windowBg: "#282c34", sidebarBg: "#21252b", sidebarHeaderBg: "#1b1f27",
            menuBarBg: "#21252b", chatBg: "#282c34", inputBg: "#2c313c",
            inputBorder: "#3e4451", inputBorderFocus: "#528bff",
            nickListBg: "#21252b", nickListHeaderBg: "#1b1f27",
            topicBg: "#21252b", separator: "#3e4451",
            textPrimary: "#abb2bf", textSecondary: "#b0b7c3", textMuted: "#5c6370",
            textInput: "#abb2bf", placeholder: "#5c6370",
            highlight: "#3e4451", highlightText: "#d7dae0",
            hoverBg: "#2c313c", selectedBg: "#3e4451", selectedText: "#d7dae0",
            buttonBg: "#528bff", buttonPressed: "#6fa0ff", buttonDisabled: "#3e4451",
            buttonText: "#fff", buttonTextDisabled: "#5c6370",
            dialogBg: "#282c34", dialogBorder: "#3e4451", dialogFieldBg: "#2c313c", dialogFieldBorder: "#3e4451",
            menuBg: "#282c34", menuText: "#abb2bf", menuHighlight: "#3e4451", menuHighlightText: "#d7dae0",
            nickOwner: "#e06c75", nickAdmin: "#d19a66", nickOp: "#98c379",
            nickHalfOp: "#c678dd", nickVoice: "#e5c07b", nickNormal: "#abb2bf",
            danger: "#e06c75"
        },
        // 9 - One Light
        {
            name: "One Light",
            windowBg: "#fafafa", sidebarBg: "#eaeaeb", sidebarHeaderBg: "#dcdcdd",
            menuBarBg: "#eaeaeb", chatBg: "#fafafa", inputBg: "#eaeaeb",
            inputBorder: "#d0d0d0", inputBorderFocus: "#526fff",
            nickListBg: "#eaeaeb", nickListHeaderBg: "#dcdcdd",
            topicBg: "#eaeaeb", separator: "#d0d0d0",
            textPrimary: "#383a42", textSecondary: "#4b4d55", textMuted: "#a0a1a7",
            textInput: "#383a42", placeholder: "#a0a1a7",
            highlight: "#d0d0d0", highlightText: "#383a42",
            hoverBg: "#d0d0d0", selectedBg: "#526fff", selectedText: "#fff",
            buttonBg: "#526fff", buttonPressed: "#6f87ff", buttonDisabled: "#d0d0d0",
            buttonText: "#fff", buttonTextDisabled: "#a0a1a7",
            dialogBg: "#fafafa", dialogBorder: "#d0d0d0", dialogFieldBg: "#eaeaeb", dialogFieldBorder: "#d0d0d0",
            menuBg: "#fafafa", menuText: "#383a42", menuHighlight: "#d0d0d0", menuHighlightText: "#383a42",
            nickOwner: "#e45649", nickAdmin: "#c18401", nickOp: "#50a14f",
            nickHalfOp: "#a626a4", nickVoice: "#986801", nickNormal: "#383a42",
            danger: "#e45649"
        },
        // 10 - Catppuccin Mocha
        {
            name: "Catppuccin Mocha",
            windowBg: "#1e1e2e", sidebarBg: "#181825", sidebarHeaderBg: "#11111b",
            menuBarBg: "#181825", chatBg: "#1e1e2e", inputBg: "#313244",
            inputBorder: "#45475a", inputBorderFocus: "#cba6f7",
            nickListBg: "#181825", nickListHeaderBg: "#11111b",
            topicBg: "#181825", separator: "#45475a",
            textPrimary: "#cdd6f4", textSecondary: "#bac2de", textMuted: "#6c7086",
            textInput: "#cdd6f4", placeholder: "#6c7086",
            highlight: "#45475a", highlightText: "#cdd6f4",
            hoverBg: "#313244", selectedBg: "#45475a", selectedText: "#cdd6f4",
            buttonBg: "#cba6f7", buttonPressed: "#d8bbff", buttonDisabled: "#45475a",
            buttonText: "#1e1e2e", buttonTextDisabled: "#6c7086",
            dialogBg: "#1e1e2e", dialogBorder: "#45475a", dialogFieldBg: "#313244", dialogFieldBorder: "#45475a",
            menuBg: "#1e1e2e", menuText: "#cdd6f4", menuHighlight: "#45475a", menuHighlightText: "#cdd6f4",
            nickOwner: "#f38ba8", nickAdmin: "#fab387", nickOp: "#a6e3a1",
            nickHalfOp: "#cba6f7", nickVoice: "#f9e2af", nickNormal: "#cdd6f4",
            danger: "#f38ba8"
        },
        // 11 - Catppuccin Latte
        {
            name: "Catppuccin Latte",
            windowBg: "#eff1f5", sidebarBg: "#e6e9ef", sidebarHeaderBg: "#dce0e8",
            menuBarBg: "#e6e9ef", chatBg: "#eff1f5", inputBg: "#e6e9ef",
            inputBorder: "#ccd0da", inputBorderFocus: "#8839ef",
            nickListBg: "#e6e9ef", nickListHeaderBg: "#dce0e8",
            topicBg: "#e6e9ef", separator: "#ccd0da",
            textPrimary: "#4c4f69", textSecondary: "#5c5f77", textMuted: "#9ca0b0",
            textInput: "#4c4f69", placeholder: "#9ca0b0",
            highlight: "#ccd0da", highlightText: "#4c4f69",
            hoverBg: "#dce0e8", selectedBg: "#8839ef", selectedText: "#eff1f5",
            buttonBg: "#8839ef", buttonPressed: "#9f52ff", buttonDisabled: "#ccd0da",
            buttonText: "#eff1f5", buttonTextDisabled: "#9ca0b0",
            dialogBg: "#eff1f5", dialogBorder: "#ccd0da", dialogFieldBg: "#e6e9ef", dialogFieldBorder: "#ccd0da",
            menuBg: "#eff1f5", menuText: "#4c4f69", menuHighlight: "#ccd0da", menuHighlightText: "#4c4f69",
            nickOwner: "#d20f39", nickAdmin: "#fe640b", nickOp: "#40a02b",
            nickHalfOp: "#8839ef", nickVoice: "#df8e1d", nickNormal: "#4c4f69",
            danger: "#d20f39"
        },
        // 12 - Tokyo Night
        {
            name: "Tokyo Night",
            windowBg: "#1a1b26", sidebarBg: "#16161e", sidebarHeaderBg: "#13131a",
            menuBarBg: "#16161e", chatBg: "#1a1b26", inputBg: "#24283b",
            inputBorder: "#3b4261", inputBorderFocus: "#7aa2f7",
            nickListBg: "#16161e", nickListHeaderBg: "#13131a",
            topicBg: "#16161e", separator: "#3b4261",
            textPrimary: "#a9b1d6", textSecondary: "#c0caf5", textMuted: "#565f89",
            textInput: "#c0caf5", placeholder: "#565f89",
            highlight: "#3b4261", highlightText: "#c0caf5",
            hoverBg: "#24283b", selectedBg: "#3b4261", selectedText: "#c0caf5",
            buttonBg: "#7aa2f7", buttonPressed: "#89b4fa", buttonDisabled: "#3b4261",
            buttonText: "#1a1b26", buttonTextDisabled: "#565f89",
            dialogBg: "#1a1b26", dialogBorder: "#3b4261", dialogFieldBg: "#24283b", dialogFieldBorder: "#3b4261",
            menuBg: "#1a1b26", menuText: "#a9b1d6", menuHighlight: "#3b4261", menuHighlightText: "#c0caf5",
            nickOwner: "#f7768e", nickAdmin: "#ff9e64", nickOp: "#9ece6a",
            nickHalfOp: "#bb9af7", nickVoice: "#e0af68", nickNormal: "#a9b1d6",
            danger: "#f7768e"
        },
        // 13 - Material Dark
        {
            name: "Material Dark",
            windowBg: "#212121", sidebarBg: "#1a1a1a", sidebarHeaderBg: "#151515",
            menuBarBg: "#1a1a1a", chatBg: "#212121", inputBg: "#303030",
            inputBorder: "#424242", inputBorderFocus: "#82aaff",
            nickListBg: "#1a1a1a", nickListHeaderBg: "#151515",
            topicBg: "#1a1a1a", separator: "#424242",
            textPrimary: "#eeffff", textSecondary: "#b0bec5", textMuted: "#546e7a",
            textInput: "#eeffff", placeholder: "#546e7a",
            highlight: "#424242", highlightText: "#eeffff",
            hoverBg: "#303030", selectedBg: "#424242", selectedText: "#eeffff",
            buttonBg: "#82aaff", buttonPressed: "#99bbff", buttonDisabled: "#424242",
            buttonText: "#212121", buttonTextDisabled: "#546e7a",
            dialogBg: "#212121", dialogBorder: "#424242", dialogFieldBg: "#303030", dialogFieldBorder: "#424242",
            menuBg: "#212121", menuText: "#eeffff", menuHighlight: "#424242", menuHighlightText: "#eeffff",
            nickOwner: "#f07178", nickAdmin: "#f78c6c", nickOp: "#c3e88d",
            nickHalfOp: "#c792ea", nickVoice: "#ffcb6b", nickNormal: "#b0bec5",
            danger: "#f07178"
        },
        // 14 - Cyberpunk
        {
            name: "Cyberpunk",
            windowBg: "#0a0a1a", sidebarBg: "#0d0d22", sidebarHeaderBg: "#08081a",
            menuBarBg: "#0d0d22", chatBg: "#0a0a1a", inputBg: "#161633",
            inputBorder: "#2a2a55", inputBorderFocus: "#ff00ff",
            nickListBg: "#0d0d22", nickListHeaderBg: "#08081a",
            topicBg: "#0d0d22", separator: "#2a2a55",
            textPrimary: "#00ffcc", textSecondary: "#00ddbb", textMuted: "#336666",
            textInput: "#00ffcc", placeholder: "#336666",
            highlight: "#2a2a55", highlightText: "#ff00ff",
            hoverBg: "#161633", selectedBg: "#330066", selectedText: "#ff00ff",
            buttonBg: "#ff00ff", buttonPressed: "#ff44ff", buttonDisabled: "#2a2a55",
            buttonText: "#0a0a1a", buttonTextDisabled: "#336666",
            dialogBg: "#0a0a1a", dialogBorder: "#ff00ff", dialogFieldBg: "#161633", dialogFieldBorder: "#2a2a55",
            menuBg: "#0a0a1a", menuText: "#00ffcc", menuHighlight: "#330066", menuHighlightText: "#ff00ff",
            nickOwner: "#ff0055", nickAdmin: "#ff6600", nickOp: "#00ff66",
            nickHalfOp: "#ff00ff", nickVoice: "#ffff00", nickNormal: "#00ffcc",
            danger: "#ff0055"
        },
        // 15 - Retro Green (classic terminal)
        {
            name: "Retro Green",
            windowBg: "#0a0a0a", sidebarBg: "#080808", sidebarHeaderBg: "#050505",
            menuBarBg: "#080808", chatBg: "#0a0a0a", inputBg: "#111111",
            inputBorder: "#003300", inputBorderFocus: "#00ff00",
            nickListBg: "#080808", nickListHeaderBg: "#050505",
            topicBg: "#080808", separator: "#003300",
            textPrimary: "#00cc00", textSecondary: "#00aa00", textMuted: "#005500",
            textInput: "#00ff00", placeholder: "#005500",
            highlight: "#003300", highlightText: "#00ff00",
            hoverBg: "#0f1f0f", selectedBg: "#003300", selectedText: "#00ff00",
            buttonBg: "#006600", buttonPressed: "#009900", buttonDisabled: "#1a1a1a",
            buttonText: "#00ff00", buttonTextDisabled: "#005500",
            dialogBg: "#0a0a0a", dialogBorder: "#003300", dialogFieldBg: "#111111", dialogFieldBorder: "#003300",
            menuBg: "#0a0a0a", menuText: "#00cc00", menuHighlight: "#003300", menuHighlightText: "#00ff00",
            nickOwner: "#ff3333", nickAdmin: "#ff9900", nickOp: "#00ff00",
            nickHalfOp: "#00ccff", nickVoice: "#ffff00", nickNormal: "#00cc00",
            danger: "#ff3333"
        },
        // 16 - Retro Amber
        {
            name: "Retro Amber",
            windowBg: "#0a0800", sidebarBg: "#080600", sidebarHeaderBg: "#050400",
            menuBarBg: "#080600", chatBg: "#0a0800", inputBg: "#111000",
            inputBorder: "#332200", inputBorderFocus: "#ffaa00",
            nickListBg: "#080600", nickListHeaderBg: "#050400",
            topicBg: "#080600", separator: "#332200",
            textPrimary: "#cc8800", textSecondary: "#aa7700", textMuted: "#554400",
            textInput: "#ffaa00", placeholder: "#554400",
            highlight: "#332200", highlightText: "#ffaa00",
            hoverBg: "#1a1100", selectedBg: "#332200", selectedText: "#ffaa00",
            buttonBg: "#664400", buttonPressed: "#885500", buttonDisabled: "#1a1100",
            buttonText: "#ffaa00", buttonTextDisabled: "#554400",
            dialogBg: "#0a0800", dialogBorder: "#332200", dialogFieldBg: "#111000", dialogFieldBorder: "#332200",
            menuBg: "#0a0800", menuText: "#cc8800", menuHighlight: "#332200", menuHighlightText: "#ffaa00",
            nickOwner: "#ff3333", nickAdmin: "#ff6600", nickOp: "#ffaa00",
            nickHalfOp: "#cc8800", nickVoice: "#ffdd00", nickNormal: "#cc8800",
            danger: "#ff3333"
        },
        // 17 - Zenburn
        {
            name: "Zenburn",
            windowBg: "#3f3f3f", sidebarBg: "#383838", sidebarHeaderBg: "#333333",
            menuBarBg: "#3f3f3f", chatBg: "#3f3f3f", inputBg: "#4f4f4f",
            inputBorder: "#5f5f5f", inputBorderFocus: "#8cd0d3",
            nickListBg: "#383838", nickListHeaderBg: "#333333",
            topicBg: "#3f3f3f", separator: "#5f5f5f",
            textPrimary: "#dcdccc", textSecondary: "#c0c0b0", textMuted: "#7f9f7f",
            textInput: "#dcdccc", placeholder: "#7f9f7f",
            highlight: "#5f5f5f", highlightText: "#dcdccc",
            hoverBg: "#4f4f4f", selectedBg: "#5f5f5f", selectedText: "#dcdccc",
            buttonBg: "#7f9f7f", buttonPressed: "#8caf8c", buttonDisabled: "#5f5f5f",
            buttonText: "#3f3f3f", buttonTextDisabled: "#7f9f7f",
            dialogBg: "#3f3f3f", dialogBorder: "#5f5f5f", dialogFieldBg: "#4f4f4f", dialogFieldBorder: "#5f5f5f",
            menuBg: "#3f3f3f", menuText: "#dcdccc", menuHighlight: "#5f5f5f", menuHighlightText: "#dcdccc",
            nickOwner: "#cc9393", nickAdmin: "#dfaf8f", nickOp: "#7f9f7f",
            nickHalfOp: "#dc8cc3", nickVoice: "#f0dfaf", nickNormal: "#dcdccc",
            danger: "#cc9393"
        },
        // 18 - Ayu Dark
        {
            name: "Ayu Dark",
            windowBg: "#0b0e14", sidebarBg: "#0d1017", sidebarHeaderBg: "#0a0d13",
            menuBarBg: "#0d1017", chatBg: "#0b0e14", inputBg: "#131721",
            inputBorder: "#1c2433", inputBorderFocus: "#e6b450",
            nickListBg: "#0d1017", nickListHeaderBg: "#0a0d13",
            topicBg: "#0d1017", separator: "#1c2433",
            textPrimary: "#bfbdb6", textSecondary: "#acaaa3", textMuted: "#565b66",
            textInput: "#bfbdb6", placeholder: "#565b66",
            highlight: "#1c2433", highlightText: "#bfbdb6",
            hoverBg: "#131721", selectedBg: "#1c2433", selectedText: "#bfbdb6",
            buttonBg: "#e6b450", buttonPressed: "#f0c565", buttonDisabled: "#1c2433",
            buttonText: "#0b0e14", buttonTextDisabled: "#565b66",
            dialogBg: "#0b0e14", dialogBorder: "#1c2433", dialogFieldBg: "#131721", dialogFieldBorder: "#1c2433",
            menuBg: "#0b0e14", menuText: "#bfbdb6", menuHighlight: "#1c2433", menuHighlightText: "#bfbdb6",
            nickOwner: "#f07178", nickAdmin: "#ff8f40", nickOp: "#aad94c",
            nickHalfOp: "#d2a6ff", nickVoice: "#e6b450", nickNormal: "#bfbdb6",
            danger: "#f07178"
        },
        // 19 - GitHub Dark
        {
            name: "GitHub Dark",
            windowBg: "#0d1117", sidebarBg: "#161b22", sidebarHeaderBg: "#0d1117",
            menuBarBg: "#161b22", chatBg: "#0d1117", inputBg: "#21262d",
            inputBorder: "#30363d", inputBorderFocus: "#58a6ff",
            nickListBg: "#161b22", nickListHeaderBg: "#0d1117",
            topicBg: "#161b22", separator: "#30363d",
            textPrimary: "#c9d1d9", textSecondary: "#b1bac4", textMuted: "#484f58",
            textInput: "#c9d1d9", placeholder: "#484f58",
            highlight: "#30363d", highlightText: "#c9d1d9",
            hoverBg: "#21262d", selectedBg: "#30363d", selectedText: "#c9d1d9",
            buttonBg: "#238636", buttonPressed: "#2ea043", buttonDisabled: "#30363d",
            buttonText: "#fff", buttonTextDisabled: "#484f58",
            dialogBg: "#0d1117", dialogBorder: "#30363d", dialogFieldBg: "#21262d", dialogFieldBorder: "#30363d",
            menuBg: "#0d1117", menuText: "#c9d1d9", menuHighlight: "#30363d", menuHighlightText: "#c9d1d9",
            nickOwner: "#f85149", nickAdmin: "#d29922", nickOp: "#3fb950",
            nickHalfOp: "#bc8cff", nickVoice: "#d29922", nickNormal: "#c9d1d9",
            danger: "#f85149"
        },
        // 20 - Midnight Blue
        {
            name: "Midnight Blue",
            windowBg: "#0f1729", sidebarBg: "#0b1220", sidebarHeaderBg: "#080e1a",
            menuBarBg: "#0b1220", chatBg: "#0f1729", inputBg: "#162040",
            inputBorder: "#1e2d55", inputBorderFocus: "#4488ff",
            nickListBg: "#0b1220", nickListHeaderBg: "#080e1a",
            topicBg: "#0b1220", separator: "#1e2d55",
            textPrimary: "#b0c4de", textSecondary: "#8faabe", textMuted: "#3d5577",
            textInput: "#b0c4de", placeholder: "#3d5577",
            highlight: "#1e2d55", highlightText: "#e0e8f0",
            hoverBg: "#162040", selectedBg: "#1e2d55", selectedText: "#e0e8f0",
            buttonBg: "#2255aa", buttonPressed: "#3366cc", buttonDisabled: "#1e2d55",
            buttonText: "#e0e8f0", buttonTextDisabled: "#3d5577",
            dialogBg: "#0f1729", dialogBorder: "#1e2d55", dialogFieldBg: "#162040", dialogFieldBorder: "#1e2d55",
            menuBg: "#0f1729", menuText: "#b0c4de", menuHighlight: "#1e2d55", menuHighlightText: "#e0e8f0",
            nickOwner: "#ff4466", nickAdmin: "#ff8844", nickOp: "#44cc88",
            nickHalfOp: "#aa88ff", nickVoice: "#eebb44", nickNormal: "#b0c4de",
            danger: "#ff4466"
        },
        // 21 - Rosé Pine
        {
            name: "Rosé Pine",
            windowBg: "#191724", sidebarBg: "#1f1d2e", sidebarHeaderBg: "#191724",
            menuBarBg: "#1f1d2e", chatBg: "#191724", inputBg: "#26233a",
            inputBorder: "#403d52", inputBorderFocus: "#c4a7e7",
            nickListBg: "#1f1d2e", nickListHeaderBg: "#191724",
            topicBg: "#1f1d2e", separator: "#403d52",
            textPrimary: "#e0def4", textSecondary: "#d0cee8", textMuted: "#6e6a86",
            textInput: "#e0def4", placeholder: "#6e6a86",
            highlight: "#403d52", highlightText: "#e0def4",
            hoverBg: "#26233a", selectedBg: "#403d52", selectedText: "#e0def4",
            buttonBg: "#c4a7e7", buttonPressed: "#d2b9f0", buttonDisabled: "#403d52",
            buttonText: "#191724", buttonTextDisabled: "#6e6a86",
            dialogBg: "#191724", dialogBorder: "#403d52", dialogFieldBg: "#26233a", dialogFieldBorder: "#403d52",
            menuBg: "#191724", menuText: "#e0def4", menuHighlight: "#403d52", menuHighlightText: "#e0def4",
            nickOwner: "#eb6f92", nickAdmin: "#f6c177", nickOp: "#31748f",
            nickHalfOp: "#c4a7e7", nickVoice: "#f6c177", nickNormal: "#e0def4",
            danger: "#eb6f92"
        },
        // 22 - Everforest Dark
        {
            name: "Everforest Dark",
            windowBg: "#2d353b", sidebarBg: "#272e33", sidebarHeaderBg: "#232a2e",
            menuBarBg: "#272e33", chatBg: "#2d353b", inputBg: "#343f44",
            inputBorder: "#475258", inputBorderFocus: "#a7c080",
            nickListBg: "#272e33", nickListHeaderBg: "#232a2e",
            topicBg: "#272e33", separator: "#475258",
            textPrimary: "#d3c6aa", textSecondary: "#c5b99a", textMuted: "#7a8478",
            textInput: "#d3c6aa", placeholder: "#7a8478",
            highlight: "#475258", highlightText: "#d3c6aa",
            hoverBg: "#343f44", selectedBg: "#475258", selectedText: "#d3c6aa",
            buttonBg: "#a7c080", buttonPressed: "#b7d090", buttonDisabled: "#475258",
            buttonText: "#2d353b", buttonTextDisabled: "#7a8478",
            dialogBg: "#2d353b", dialogBorder: "#475258", dialogFieldBg: "#343f44", dialogFieldBorder: "#475258",
            menuBg: "#2d353b", menuText: "#d3c6aa", menuHighlight: "#475258", menuHighlightText: "#d3c6aa",
            nickOwner: "#e67e80", nickAdmin: "#e69875", nickOp: "#a7c080",
            nickHalfOp: "#d699b6", nickVoice: "#dbbc7f", nickNormal: "#d3c6aa",
            danger: "#e67e80"
        },
        // 23 - Ice
        {
            name: "Ice",
            windowBg: "#e8eef4", sidebarBg: "#dce3ea", sidebarHeaderBg: "#d0d8e0",
            menuBarBg: "#dce3ea", chatBg: "#e8eef4", inputBg: "#dce3ea",
            inputBorder: "#b8c4d0", inputBorderFocus: "#5588cc",
            nickListBg: "#dce3ea", nickListHeaderBg: "#d0d8e0",
            topicBg: "#dce3ea", separator: "#b8c4d0",
            textPrimary: "#2c3e50", textSecondary: "#3d5066", textMuted: "#8899aa",
            textInput: "#2c3e50", placeholder: "#8899aa",
            highlight: "#b8c4d0", highlightText: "#2c3e50",
            hoverBg: "#d0d8e0", selectedBg: "#5588cc", selectedText: "#fff",
            buttonBg: "#5588cc", buttonPressed: "#6699dd", buttonDisabled: "#b8c4d0",
            buttonText: "#fff", buttonTextDisabled: "#8899aa",
            dialogBg: "#e8eef4", dialogBorder: "#b8c4d0", dialogFieldBg: "#dce3ea", dialogFieldBorder: "#b8c4d0",
            menuBg: "#e8eef4", menuText: "#2c3e50", menuHighlight: "#b8c4d0", menuHighlightText: "#2c3e50",
            nickOwner: "#cc3333", nickAdmin: "#cc6633", nickOp: "#338833",
            nickHalfOp: "#6633aa", nickVoice: "#997722", nickNormal: "#2c3e50",
            danger: "#cc3333"
        },
        // 24 - High Contrast
        {
            name: "High Contrast",
            windowBg: "#000000", sidebarBg: "#000000", sidebarHeaderBg: "#000000",
            menuBarBg: "#000000", chatBg: "#000000", inputBg: "#111111",
            inputBorder: "#ffffff", inputBorderFocus: "#ffff00",
            nickListBg: "#000000", nickListHeaderBg: "#000000",
            topicBg: "#000000", separator: "#ffffff",
            textPrimary: "#ffffff", textSecondary: "#ffffff", textMuted: "#aaaaaa",
            textInput: "#ffffff", placeholder: "#888888",
            highlight: "#ffff00", highlightText: "#000000",
            hoverBg: "#222222", selectedBg: "#ffff00", selectedText: "#000000",
            buttonBg: "#ffffff", buttonPressed: "#ffff00", buttonDisabled: "#333333",
            buttonText: "#000000", buttonTextDisabled: "#666666",
            dialogBg: "#000000", dialogBorder: "#ffffff", dialogFieldBg: "#111111", dialogFieldBorder: "#ffffff",
            menuBg: "#000000", menuText: "#ffffff", menuHighlight: "#ffff00", menuHighlightText: "#000000",
            nickOwner: "#ff0000", nickAdmin: "#ff8800", nickOp: "#00ff00",
            nickHalfOp: "#ff00ff", nickVoice: "#ffff00", nickNormal: "#ffffff",
            danger: "#ff0000"
        }
    ]

    function setTheme(index) {
        if (index >= 0 && index < themes.length)
            currentThemeIndex = index
    }

    function nextTheme() {
        currentThemeIndex = (currentThemeIndex + 1) % themes.length
    }

    function prevTheme() {
        currentThemeIndex = (currentThemeIndex - 1 + themes.length) % themes.length
    }

    // Returns array of theme name strings
    property var themeNames: {
        var names = []
        for (var i = 0; i < themes.length; i++)
            names.push(themes[i].name)
        return names
    }
}
