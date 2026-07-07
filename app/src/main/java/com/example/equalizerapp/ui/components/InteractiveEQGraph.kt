package com.example.equalizerapp.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import com.example.equalizerapp.MainActivity
import kotlin.math.*

@Composable
fun InteractiveEQGraph(
    responseCurve: FloatArray,
    bandConfigs: List<MainActivity.BandConfig>,
    onBandChange: (Int, Float, Float) -> Unit,
    modifier: Modifier = Modifier,
    accentColor: Color = Color(0xFFFF3B4E)
) {
    BoxWithConstraints(modifier = modifier.background(Color(0xFF0A0A0A))) {
        val width = constraints.maxWidth.toFloat()
        val height = constraints.maxHeight.toFloat()
        val midY = height / 2
        val scaleY = height / 40f

        Canvas(modifier = Modifier.fillMaxSize().pointerInput(bandConfigs) {
            detectDragGestures { change, _ ->
                val touchX = change.position.x
                val touchY = change.position.y
                
                var closestIndex = -1
                var minDistance = Float.MAX_VALUE
                
                bandConfigs.forEachIndexed { index, config ->
                    val bx = freqToX(config.frequency, width)
                    val by = midY - (config.gain * scaleY)
                    val dist = sqrt((touchX - bx).pow(2) + (touchY - by).pow(2))
                    // Increase hit area for easier dragging
                    if (dist < 48.dp.toPx() && dist < minDistance) {
                        minDistance = dist
                        closestIndex = index
                    }
                }

                if (closestIndex != -1) {
                    val newFreq = xToFreq(touchX, width)
                    val newGain = (midY - touchY) / scaleY
                    onBandChange(closestIndex, newFreq.coerceIn(20f, 20000f), newGain.coerceIn(-15f, 15f))
                }
            }
        }) {
            // High-end Grid
            for (db in -15..15 step 5) {
                val y = midY - (db * scaleY)
                drawLine(
                    color = Color.White.copy(alpha = 0.05f),
                    start = Offset(0f, y),
                    end = Offset(width, y),
                    strokeWidth = 1.dp.toPx()
                )
            }
            
            // Vertical Freq markers (Rough logarithmic)
            val markers = listOf(20f, 100f, 1000f, 10000f, 20000f)
            markers.forEach { freq ->
                val x = freqToX(freq, width)
                drawLine(
                    color = Color.White.copy(alpha = 0.03f),
                    start = Offset(x, 0f),
                    end = Offset(x, height),
                    strokeWidth = 1.dp.toPx()
                )
            }

            // Curve with Glow Effect
            if (responseCurve.isNotEmpty()) {
                val path = Path()
                for (i in responseCurve.indices) {
                    val x = (i.toFloat() / (responseCurve.size - 1)) * width
                    val y = (midY - (responseCurve[i] * scaleY)).coerceIn(0f, height)
                    if (i == 0) path.moveTo(x, y) else path.lineTo(x, y)
                }

                // Shadow/Glow under the curve
                drawPath(
                    path = path,
                    brush = Brush.verticalGradient(
                        colors = listOf(accentColor.copy(alpha = 0.2f), Color.Transparent),
                        startY = 0f,
                        endY = height
                    ),
                    style = Stroke(width = 8.dp.toPx(), cap = StrokeCap.Round)
                )

                // Main Line
                drawPath(
                    path = path,
                    color = Color.Cyan,
                    style = Stroke(width = 3.dp.toPx(), cap = StrokeCap.Round)
                )
                
                // Gradient Fill Area
                val fillPath = Path().apply {
                    addPath(path)
                    lineTo(width, height)
                    lineTo(0f, height)
                    close()
                }
                drawPath(
                    path = fillPath,
                    brush = Brush.verticalGradient(
                        colors = listOf(Color.Cyan.copy(alpha = 0.15f), Color.Transparent),
                        startY = midY - (15 * scaleY),
                        endY = height
                    )
                )
            }

            // Interactive Nodes (Knobs on graph)
            bandConfigs.forEach { config ->
                val nx = freqToX(config.frequency, width)
                val ny = midY - (config.gain * scaleY)
                
                // Halo Glow
                drawCircle(
                    brush = Brush.radialGradient(
                        colors = listOf(accentColor.copy(alpha = 0.4f), Color.Transparent),
                        center = Offset(nx, ny),
                        radius = 20.dp.toPx()
                    ),
                    radius = 20.dp.toPx(),
                    center = Offset(nx, ny)
                )
                
                // Core
                drawCircle(
                    color = accentColor,
                    radius = 5.dp.toPx(),
                    center = Offset(nx, ny)
                )
                drawCircle(
                    color = Color.White,
                    radius = 2.dp.toPx(),
                    center = Offset(nx, ny)
                )
            }
        }
    }
}

private fun freqToX(freq: Float, width: Float): Float {
    val logMin = ln(20f)
    val logMax = ln(20000f)
    return ((ln(freq.coerceAtLeast(20f)) - logMin) / (logMax - logMin)) * width
}

private fun xToFreq(x: Float, width: Float): Float {
    val logMin = ln(20f)
    val logMax = ln(20000f)
    val logFreq = logMin + (x / width) * (logMax - logMin)
    return exp(logFreq)
}
