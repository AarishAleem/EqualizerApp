package com.example.equalizerapp.ui.theme

import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

val ElectricBlue = Color(0xFF007BFF)
val NeonPurple = Color(0xFF9D00FF)
val EmeraldGreen = Color(0xFF00C853)
val GlassWhite = Color(0x1AFFFFFF)
val StudioRed = Color(0xFFFF3B4E)

private val DarkColorScheme = darkColorScheme(
    primary = StudioRed,
    secondary = Color(0xFF181818),
    tertiary = ElectricBlue,
    background = Color(0xFF111111),
    surface = Color(0xFF202020),
    onPrimary = Color.White,
    onSecondary = Color.White,
    onBackground = Color.White,
    onSurface = Color(0xFFE0E0E0)
)

@Composable
fun PowerEQTheme(
    content: @Composable () -> Unit
) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        typography = Typography,
        content = content
    )
}