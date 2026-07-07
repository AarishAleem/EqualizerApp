package com.example.equalizerapp.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.equalizerapp.ui.components.StudioCard
import com.example.equalizerapp.ui.components.StudioKnob

@Composable
fun EffectsScreen(
    masterVolume: Float,
    onVolumeChange: (Float) -> Unit,
    surroundStrength: Float,
    onSurroundChange: (Float) -> Unit,
    channelDelay: Float,
    onDelayChange: (Float) -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF111111))
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "Master DSP",
            style = MaterialTheme.typography.headlineLarge,
            color = Color.White,
            modifier = Modifier.padding(bottom = 32.dp)
        )

        StudioCard(
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    "TOTAL OUTPUT",
                    style = MaterialTheme.typography.labelMedium,
                    color = Color.Gray,
                    letterSpacing = 2.sp
                )
                Spacer(Modifier.height(16.dp))
                StudioKnob(
                    value = masterVolume,
                    onValueChange = onVolumeChange,
                    range = 0f..100f,
                    label = "MAIN VOLUME",
                    size = 150f,
                    accentColor = Color(0xFFFF3B4E)
                )
            }
        }

        Spacer(Modifier.height(24.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(24.dp)
        ) {
            StudioCard(
                modifier = Modifier.weight(1f)
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    StudioKnob(
                        value = surroundStrength,
                        onValueChange = onSurroundChange,
                        range = 0f..1000f,
                        label = "SPATIAL",
                        size = 100f,
                        accentColor = Color(0xFF007BFF)
                    )
                }
            }

            StudioCard(
                modifier = Modifier.weight(1f)
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    StudioKnob(
                        value = channelDelay,
                        onValueChange = onDelayChange,
                        range = 0f..30f,
                        label = "DELAY MS",
                        size = 100f,
                        accentColor = Color(0xFF9D00FF)
                    )
                }
            }
        }

        Spacer(Modifier.weight(1f))

        Text(
            text = "PREMIUM AUDIO ENGINE ACTIVE",
            style = MaterialTheme.typography.labelSmall,
            color = Color.DarkGray,
            letterSpacing = 1.sp
        )
    }
}
