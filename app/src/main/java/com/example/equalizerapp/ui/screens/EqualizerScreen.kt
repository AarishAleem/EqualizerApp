package com.example.equalizerapp.ui.screens

import androidx.compose.animation.*
import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.equalizerapp.MainActivity
import com.example.equalizerapp.ui.components.*
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun EqualizerScreen(
    bandConfigs: SnapshotStateList<MainActivity.BandConfig>,
    graphData: FloatArray,
    realTimeFft: FloatArray,
    masterGain: Float,
    presets: List<MainActivity.Preset>,
    currentPresetName: String,
    onMasterGainChanged: (Float) -> Unit,
    onAddBand: () -> Unit,
    onRemoveBand: (Int) -> Unit,
    onConfigChanged: () -> Unit,
    onPresetSelected: (MainActivity.Preset) -> Unit,
    onSavePreset: (String) -> Unit,
    onResetPreset: () -> Unit
) {
    var showSaveDialog by remember { mutableStateOf(false) }
    var newPresetName by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(MaterialTheme.colorScheme.background)
    ) {
        // Interactive Graph
        InteractiveEQGraph(
            responseCurve = graphData,
            bandConfigs = bandConfigs,
            onBandChange = { index, freq, gain ->
                bandConfigs[index].frequency = freq
                bandConfigs[index].gain = gain
                onConfigChanged()
            },
            modifier = Modifier
                .fillMaxWidth()
                .height(280.dp)
                .clip(RoundedCornerShape(bottomStart = 32.dp, bottomEnd = 32.dp))
        )

        // Presets Row
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 16.dp)
                .horizontalScroll(rememberScrollState())
                .padding(horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            presets.forEach { preset ->
                val isSelected = currentPresetName == preset.name
                Surface(
                    onClick = { onPresetSelected(preset) },
                    shape = RoundedCornerShape(12.dp),
                    color = if (isSelected) Color(0xFFFF3B4E).copy(alpha = 0.2f) else Color.White.copy(alpha = 0.05f),
                    border = BorderStroke(
                        width = 1.dp,
                        color = if (isSelected) Color(0xFFFF3B4E) else Color.Transparent
                    ),
                    modifier = Modifier.padding(end = 12.dp)
                ) {
                    Text(
                        text = preset.name.uppercase(),
                        modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
                        style = MaterialTheme.typography.labelMedium,
                        color = if (isSelected) Color.White else Color.Gray,
                        fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal
                    )
                }
            }
            
            IconButton(
                onClick = onResetPreset,
                modifier = Modifier
                    .padding(end = 12.dp)
                    .clip(CircleShape)
                    .background(Color.White.copy(alpha = 0.1f))
            ) {
                Icon(Icons.Default.Refresh, "Reset Preset", tint = Color.LightGray)
            }

            IconButton(
                onClick = { showSaveDialog = true },
                modifier = Modifier
                    .clip(CircleShape)
                    .background(Color.White.copy(alpha = 0.1f))
            ) {
                Icon(Icons.Default.Add, null, tint = Color.Cyan)
            }
        }

        if (showSaveDialog) {
            AlertDialog(
                onDismissRequest = { showSaveDialog = false },
                title = { Text("Save Custom Preset") },
                text = {
                    OutlinedTextField(
                        value = newPresetName,
                        onValueChange = { newPresetName = it },
                        label = { Text("Name") },
                        singleLine = true,
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedBorderColor = Color.Cyan,
                            cursorColor = Color.Cyan
                        )
                    )
                },
                confirmButton = {
                    TextButton(onClick = {
                        if (newPresetName.isNotBlank()) {
                            onSavePreset(newPresetName)
                            showSaveDialog = false
                        }
                    }) { Text("SAVE", color = Color.Cyan) }
                },
                dismissButton = {
                    TextButton(onClick = { showSaveDialog = false }) { Text("CANCEL") }
                }
            )
        }

        // Master Gain Knob in a special section
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.Center,
            verticalAlignment = Alignment.CenterVertically
        ) {
            StudioKnob(
                value = masterGain,
                onValueChange = onMasterGainChanged,
                range = -15f..15f,
                label = "MASTER GAIN",
                size = 110f
            )
        }

        // Filter Bands List
        LazyColumn(
            modifier = Modifier
                .weight(1f)
                .padding(horizontal = 16.dp)
        ) {
            item {
                Button(
                    onClick = onAddBand,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(bottom = 16.dp),
                    shape = RoundedCornerShape(16.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = Color.DarkGray)
                ) {
                    Icon(Icons.Default.Add, null)
                    Spacer(Modifier.width(8.dp))
                    Text("ADD NEW FILTER BAND", fontWeight = FontWeight.Bold)
                }
            }

            itemsIndexed(
                items = bandConfigs,
                key = { _, config -> config.hashCode() } // Unique key for animation
            ) { index, config ->
                BandCard(
                    index = index,
                    config = config,
                    onRemove = { onRemoveBand(index) },
                    onChanged = onConfigChanged
                )
            }
            
            item { Spacer(Modifier.height(32.dp)) }
        }
    }
}

@Composable
fun BandCard(
    index: Int,
    config: MainActivity.BandConfig,
    onRemove: () -> Unit,
    onChanged: () -> Unit
) {
    StudioCard(
        modifier = Modifier.fillMaxWidth().padding(vertical = 8.dp)
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(
                text = "BAND ${index + 1}",
                style = MaterialTheme.typography.titleMedium,
                color = Color.Cyan,
                fontWeight = FontWeight.Bold
            )
            
            Spacer(Modifier.weight(1f))
            
            FilterTypeSelector(
                selectedIdx = config.typeIndex,
                onSelected = {
                    config.typeIndex = it
                    onChanged()
                }
            )
            
            IconButton(onClick = onRemove, modifier = Modifier.size(32.dp)) {
                Icon(Icons.Default.Close, null, tint = Color.Red.copy(alpha = 0.6f))
            }
        }

        Spacer(Modifier.height(16.dp))

        SliderItem(
            label = "FREQ",
            value = config.frequency,
            range = 20f..20000f,
            valueText = "${config.frequency.toInt()} Hz",
            onValueChange = {
                config.frequency = it
                onChanged()
            }
        )

        SliderItem(
            label = "GAIN",
            value = config.gain,
            range = -15f..15f,
            valueText = "${config.gain.toInt()} dB",
            onValueChange = {
                config.gain = it
                onChanged()
            }
        )

        SliderItem(
            label = "Q",
            value = config.q,
            range = 0.1f..10f,
            valueText = String.format(Locale.US, "%.1f", config.q),
            onValueChange = {
                config.q = it
                onChanged()
            }
        )
    }
}

@Composable
fun SliderItem(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    valueText: String,
    onValueChange: (Float) -> Unit
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.padding(vertical = 4.dp)
    ) {
        Text(
            text = label,
            modifier = Modifier.width(44.dp),
            style = MaterialTheme.typography.labelSmall,
            color = Color.Gray,
            fontWeight = FontWeight.Bold
        )
        ProfessionalSlider(
            value = value,
            onValueChange = onValueChange,
            range = range,
            modifier = Modifier.weight(1f)
        )
        Text(
            text = valueText,
            modifier = Modifier.width(64.dp),
            style = MaterialTheme.typography.labelSmall,
            textAlign = androidx.compose.ui.text.style.TextAlign.End,
            color = Color.White
        )
    }
}

@Composable
fun FilterTypeSelector(
    selectedIdx: Int,
    onSelected: (Int) -> Unit
) {
    val types = listOf("PEAK", "LP", "HP", "BP", "LS", "HS")
    Row(
        modifier = Modifier
            .background(Color.Black.copy(alpha = 0.4f), RoundedCornerShape(8.dp))
            .padding(2.dp)
    ) {
        types.forEachIndexed { index, name ->
            val isSelected = selectedIdx == index
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(6.dp))
                    .background(if (isSelected) Color.Cyan else Color.Transparent)
                    .clickable { onSelected(index) }
                    .padding(horizontal = 6.dp, vertical = 4.dp)
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    FilterIconSmall(index, if (isSelected) Color.Black else Color.Gray)
                    Text(
                        text = name,
                        fontSize = 8.sp,
                        color = if (isSelected) Color.Black else Color.White,
                        fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal
                    )
                }
            }
        }
    }
}

@Composable
fun FilterIconSmall(type: Int, color: Color) {
    Canvas(modifier = Modifier.size(14.dp, 7.dp)) {
        val w = size.width
        val h = size.height
        val path = Path()
        when (type) {
            0 -> { 
                path.moveTo(0f, h)
                path.quadraticBezierTo(w/2, -h * 0.4f, w, h)
            }
            1 -> { 
                path.moveTo(0f, 0f)
                path.lineTo(w * 0.5f, 0f)
                path.quadraticBezierTo(w, 0f, w, h)
            }
            2 -> { 
                path.moveTo(0f, h)
                path.quadraticBezierTo(0f, 0f, w * 0.5f, 0f)
                path.lineTo(w, 0f)
            }
            3 -> { 
                path.moveTo(0f, h)
                path.lineTo(w/2, 0f)
                path.lineTo(w, h)
            }
            4 -> { 
                path.moveTo(0f, 0f)
                path.lineTo(w * 0.3f, 0f)
                path.quadraticBezierTo(w * 0.5f, h * 0.7f, w, h * 0.7f)
            }
            5 -> { 
                path.moveTo(0f, h * 0.7f)
                path.quadraticBezierTo(w * 0.5f, h * 0.7f, w * 0.7f, 0f)
                path.lineTo(w, 0f)
            }
        }
        drawPath(path, color, style = Stroke(width = 1.5.dp.toPx(), cap = StrokeCap.Round))
    }
}
