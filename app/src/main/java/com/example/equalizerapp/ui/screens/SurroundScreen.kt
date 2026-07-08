package com.example.equalizerapp.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.equalizerapp.ui.components.StudioCard
import com.example.equalizerapp.ui.components.StudioKnob

@Composable
fun SurroundScreen(
    width: Float,
    onWidthChange: (Float) -> Unit,
    crossfeed: Float = 0f,
    onCrossfeedChange: (Float) -> Unit = {}
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "3D Surround Engine",
            style = MaterialTheme.typography.headlineMedium,
            color = Color.Cyan,
            modifier = Modifier.padding(bottom = 32.dp)
        )

        StudioCard(modifier = Modifier.fillMaxWidth()) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text("STEREO WIDENING", color = Color.Gray, fontSize = 12.sp)
                Spacer(Modifier.height(16.dp))
                StudioKnob(
                    value = width,
                    onValueChange = onWidthChange,
                    range = 0f..100f,
                    label = "WIDTH %",
                    size = 140f,
                    accentColor = Color.Cyan
                )
            }
        }

        Spacer(Modifier.height(24.dp))

        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(16.dp)) {
            StudioCard(modifier = Modifier.weight(1f)) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    StudioKnob(
                        value = crossfeed,
                        onValueChange = onCrossfeedChange,
                        range = 0f..100f,
                        label = "CROSSFEED",
                        size = 90f,
                        accentColor = Color(0xFF9D00FF)
                    )
                }
            }
            
            StudioCard(modifier = Modifier.weight(1f)) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    StudioKnob(
                        value = 0f, // Placeholder
                        onValueChange = {},
                        range = 0f..100f,
                        label = "ROOM SIZE",
                        size = 90f,
                        accentColor = Color.Yellow
                    )
                }
            }
        }
        
        Spacer(Modifier.weight(1f))
        
        Text(
            "CUSTOM C++ SPATIALIZER ACTIVE",
            style = MaterialTheme.typography.labelSmall,
            color = Color.DarkGray
        )
    }
}
