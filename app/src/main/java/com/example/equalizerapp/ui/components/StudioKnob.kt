package com.example.equalizerapp.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.*

@Composable
fun StudioKnob(
    value: Float,
    onValueChange: (Float) -> Unit,
    range: ClosedFloatingPointRange<Float>,
    label: String,
    modifier: Modifier = Modifier,
    size: Float = 100f,
    accentColor: Color = Color(0xFFFF3B4E)
) {
    // Current angle based on value
    val targetAngle = valueToAngle(value, range)
    
    // Smooth angle for display
    val animatedAngle by animateFloatAsState(
        targetValue = targetAngle,
        animationSpec = spring(dampingRatio = 0.9f, stiffness = 700f),
        label = "knobAngle"
    )

    Column(
        modifier = modifier,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(
            modifier = Modifier
                .size(size.dp)
                .pointerInput(Unit) {
                    detectDragGestures(
                        onDrag = { change, _ ->
                            change.consume()
                            
                            val touchPos = change.position
                            // Center in pixels
                            val centerPx = size.dp.toPx() / 2f
                            
                            // Vector from center to touch
                            val dx = touchPos.x - centerPx
                            val dy = touchPos.y - centerPx
                            
                            // Calculate angle in degrees
                            var angleDeg = Math.toDegrees(atan2(dy.toDouble(), dx.toDouble())).toFloat()
                            
                            // Adjust so bottom is gap, top is center of travel
                            // atan2 0 is right. 90 is down. -90 is up. 
                            // We want -150 to 150.
                            angleDeg += 90f 
                            if (angleDeg < -180f) angleDeg += 360f
                            if (angleDeg > 180f) angleDeg -= 360f
                            
                            // Allow dragging only within active range
                            if (angleDeg in -155f..155f) {
                                val newVal = angleToValue(angleDeg.coerceIn(-150f, 150f), range)
                                onValueChange(newVal)
                            }
                        }
                    )
                },
            contentAlignment = Alignment.Center
        ) {
            Canvas(modifier = Modifier.fillMaxSize()) {
                val canvasSize = size.dp.toPx()
                val center = Offset(canvasSize / 2f, canvasSize / 2f)
                val radius = canvasSize / 2.8f

                // Outer ambient shadow
                drawCircle(
                    brush = Brush.radialGradient(
                        0.6f to Color.Black.copy(alpha = 0.6f),
                        1f to Color.Transparent,
                        center = center,
                        radius = radius * 1.8f
                    ),
                    radius = radius * 1.8f,
                    center = center
                )

                // Knob Core (Matte Black Aluminum)
                drawCircle(
                    brush = Brush.linearGradient(
                        colors = listOf(Color(0xFF333333), Color(0xFF0A0A0A)),
                        start = Offset(0f, 0f),
                        end = Offset(canvasSize, canvasSize)
                    ),
                    radius = radius,
                    center = center
                )

                // Glossy Rim
                drawCircle(
                    color = Color.White.copy(alpha = 0.08f),
                    radius = radius,
                    center = center,
                    style = Stroke(width = 1.dp.toPx())
                )

                // Active Progress Arc
                val sweep = animatedAngle + 150f
                drawArc(
                    brush = Brush.sweepGradient(
                        0f to accentColor.copy(alpha = 0.2f),
                        0.5f to accentColor,
                        1f to accentColor,
                        center = center
                    ),
                    startAngle = 120f, // Map -150 to 120 on Canvas
                    sweepAngle = sweep,
                    useCenter = false,
                    topLeft = Offset(center.x - radius, center.y - radius),
                    size = Size(radius * 2, radius * 2),
                    style = Stroke(width = 3.dp.toPx(), cap = StrokeCap.Round)
                )

                // Precision Ticks
                for (i in -150..150 step 30) {
                    val rad = (i - 90).toDouble() * PI / 180.0
                    val isActive = animatedAngle >= i
                    val color = if (isActive) accentColor else Color.Gray.copy(alpha = 0.25f)
                    
                    val startR = radius + 8.dp.toPx()
                    val endR = radius + 15.dp.toPx()
                    
                    drawLine(
                        color = color,
                        start = Offset(
                            (center.x + startR * cos(rad)).toFloat(),
                            (center.y + startR * sin(rad)).toFloat()
                        ),
                        end = Offset(
                            (center.x + endR * cos(rad)).toFloat(),
                            (center.y + endR * sin(rad)).toFloat()
                        ),
                        strokeWidth = (if (isActive) 2.5 else 1.5).dp.toPx(),
                        cap = StrokeCap.Round
                    )
                }

                // LED Pointer Position
                val pRad = (animatedAngle - 90).toDouble() * PI / 180.0
                val pPos = Offset(
                    (center.x + (radius - 12.dp.toPx()) * cos(pRad)).toFloat(),
                    (center.y + (radius - 12.dp.toPx()) * sin(pRad)).toFloat()
                )
                
                // Glow
                drawCircle(
                    color = accentColor.copy(alpha = 0.4f),
                    radius = 6.dp.toPx(),
                    center = pPos
                )
                // Core
                drawCircle(
                    color = Color.White,
                    radius = 2.dp.toPx(),
                    center = pPos
                )
            }
        }
        Spacer(Modifier.height(4.dp))
        Text(
            text = label,
            style = MaterialTheme.typography.labelMedium,
            color = Color.Gray,
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            letterSpacing = 1.sp
        )
    }
}

private fun valueToAngle(value: Float, range: ClosedFloatingPointRange<Float>): Float {
    val percent = (value - range.start) / (range.endInclusive - range.start)
    return (percent * 300f) - 150f
}

private fun angleToValue(angle: Float, range: ClosedFloatingPointRange<Float>): Float {
    val percent = (angle + 150f) / 300f
    return range.start + (percent * (range.endInclusive - range.start))
}
