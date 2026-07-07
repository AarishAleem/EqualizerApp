package com.example.equalizerapp.ui.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun ProfessionalSlider(
    value: Float,
    onValueChange: (Float) -> Unit,
    range: ClosedFloatingPointRange<Float>,
    modifier: Modifier = Modifier,
    accentColor: Color = Color(0xFFFF3B4E)
) {
    Slider(
        value = value,
        onValueChange = onValueChange,
        valueRange = range,
        modifier = modifier.height(32.dp),
        colors = SliderDefaults.colors(
            thumbColor = accentColor,
            activeTrackColor = accentColor,
            inactiveTrackColor = Color.DarkGray
        )
    )
}