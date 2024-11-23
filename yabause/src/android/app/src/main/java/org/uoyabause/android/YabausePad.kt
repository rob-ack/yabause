/*  Copyright 2013 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
/*  Copyright 2019 devMiyax(smiyaxdev@gmail.com)

    This file is part of YabaSanshiro.

    YabaSanshiro is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    YabaSanshiro is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with YabaSanshiro; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
package org.uoyabause.android

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.os.Build
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import android.view.View
import android.view.View.OnTouchListener
import androidx.preference.PreferenceManager
import org.devmiyax.yabasanshiro.R
import java.lang.Math.PI
import java.lang.Math.atan2
import java.lang.Math.sqrt
import kotlin.math.pow

internal open class PadButton {
    protected var rect: RectF
    var pointId: Int
        protected set
    protected var pointid_: Int
    var back: Paint? = null
    var scale: Float
    var centerX: Float = 0.0f
    var centerY: Float = 0.0f

    val pushPaint = Paint().apply {
        style = Paint.Style.FILL // 塗りつぶしスタイルを設定
        color = 0x80FFFFFF.toInt() // 色を設定（この例では緑）
    }

    open fun recalc(){

    }

    fun updateRect(x1: Int, y1: Int, x2: Int, y2: Int) {
        rect[x1.toFloat(), y1.toFloat(), x2.toFloat()] = y2.toFloat()
        centerX = rect.centerX()
        centerY = rect.centerY()
        recalc()
    }

    fun updateRect(matrix: Matrix, x1: Int, y1: Int, x2: Int, y2: Int) {
        rect[x1.toFloat(), y1.toFloat(), x2.toFloat()] = y2.toFloat()
        matrix.mapRect(rect)
        centerX = rect.centerX()
        centerY = rect.centerY()
        recalc()
    }

    fun updateScale(scale: Float) {
        this.scale = scale
    }

    fun contains(x: Int, y: Int): Boolean {
        return rect.contains(x.toFloat(), y.toFloat())
    }

    fun intersects(r: RectF?): Boolean {
        return RectF.intersects(rect, r!!)
    }

    open fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        back = if (pointId != -1) {
            active_back
        } else {
            nomal_back
        }
    }

    fun On(index: Int) {
        pointId = index
    }

    open fun Off() {
        pointId = -1
        pointid_ = -1
    }

    fun isOn(index: Int): Boolean {
        return pointId == index
    }

    fun isOn(): Boolean {
        return pointId != -1
    }

    init {
        pointid_ = -1
        pointId = -1
        rect = RectF()
        scale = 1.0f
    }
}

internal class DPadButton : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        if ( isOn() ) {
             canvas.drawRect(rect, pushPaint!!)
        }else {
           // canvas.drawRect(rect, pushPaint!!)
        }
    }
}

internal class StartButton : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        if ( isOn() ) {

            val newWidth = rect.width() * 1.5f
            val newHeight = rect.height() * 1.5f

            // 新しいRectFオブジェクトを作成
            val drect = RectF(
                centerX - newWidth / 2,
                centerY - newHeight / 2,
                centerX + newWidth / 2,
                centerY + newHeight / 2
            )

            canvas.drawOval(drect, pushPaint)
        }else{
            //canvas.drawOval(rect, pushPaint)
        }
    }
}

internal class ActionButton(
    private val width: Int,
    private val text: String,
    private val textsize: Int,
) : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        if ( isOn() ) {
            canvas.drawCircle(rect.centerX(), rect.centerY(), width * scale * 1.5f, pushPaint)
        }else{
           //canvas.drawCircle(rect.centerX(), rect.centerY(), width * scale, pushPaint)
        }
        // front.setTextSize(textsize);
        // front.setTextAlign(Paint.Align.CENTER);
        // canvas.drawText(text, rect.centerX() , rect.centerY() , front);
    }
}


data class DpadState(var left: Boolean, var right: Boolean, var up: Boolean, var down: Boolean)

internal class Dpad(
    private val width: Int,
    private val deadZone: Int,
) : PadButton() {

    val path = Path()
    val drect = RectF()

    override fun recalc() {
        val newWidth = rect.width() * 1.5f
        val newHeight = rect.height() * 1.5f

        // 新しいRectFオブジェクトを作成
        drect.set(
            centerX - newWidth / 2,
            centerY - newHeight / 2,
            centerX + newWidth / 2,
            centerY + newHeight / 2
        )

        val outerRadius = Math.min(newWidth, newWidth) / 2f
        val innerRadius = (deadZone * scale)
        path.apply {
            reset()
            addCircle(centerX, centerY, outerRadius, Path.Direction.CW)
            addCircle(centerX, centerY, innerRadius, Path.Direction.CCW)
        }
    }

    init {
        recalc()
    }

    data class Point(val x: Double, val y: Double)

    fun normalizeRadian(radian: Double): Double {
        return ((radian % (2 * PI) + 2 * PI) % (2 * PI))
    }

    fun angleBetweenPoints(point1: Point, point2: Point): Double {
        val deltaX = point2.x - point1.x
        val deltaY = point2.y - point1.y
        return normalizeRadian(atan2(deltaY, deltaX))
    }

    fun radiansToDegrees(radians: Double): Double {
        return radians * (180 / PI)
    }

    fun degreesToRadians(degrees: Double): Double {
        return degrees * (PI / 180)
    }

    fun distance(x1: Double, y1: Double, x2: Double, y2: Double): Double {
        return sqrt((x2 - x1).pow(2) + (y2 - y1).pow(2))
    }

    var ds = DpadState(false,false,false, false)

    fun getState(x: Int, y: Int ) : DpadState {

        if( distance( centerX.toDouble(),centerY.toDouble(), x.toDouble(),y.toDouble() ) < deadZone*scale ){
            ds.up    = false
            ds.right = false
            ds.down  = false
            ds.left  = false
            return ds
        }

        val pos = degreesToRadians(22.5)
        val rad = angleBetweenPoints(  Point(centerX.toDouble(),centerY.toDouble()), Point(x.toDouble(),y.toDouble())  )
        if( rad >= 0 && rad < pos ){
            ds.up    = false
            ds.right = true
            ds.down  = false
            ds.left  = false
        }else if( rad >= pos && rad < pos + (pos*2) ){
            ds.up    = false
            ds.right = true
            ds.down  = true
            ds.left  = false
        }else if( rad >= pos + (pos*2) && rad < pos + (pos*4) ){
            ds.up    = false
            ds.right = false
            ds.down  = true
            ds.left  = false
        }else if( rad >= pos + (pos*4) && rad < pos + (pos*6) ){
            ds.up    = false
            ds.right = false
            ds.down  = true
            ds.left  = true
        }else if( rad >= pos + (pos*6) && rad < pos + (pos*8) ){
            ds.up    = false
            ds.right = false
            ds.down  = false
            ds.left  = true
        }else if( rad >= pos + (pos*8) && rad < pos + (pos*10) ){
            ds.up    = true
            ds.right = false
            ds.down  = false
            ds.left  = true
        }else if( rad >= pos + (pos*10) && rad < pos + (pos*12) ) {
            ds.up    = true
            ds.right = false
            ds.down  = false
            ds.left  = false
        }else if( rad >= pos + (pos*12) && rad < pos + (pos*14) ){
            ds.up    = true
            ds.right = true
            ds.down  = false
            ds.left  = false
        }else if( rad >= pos + (pos*14) && rad < (pos*16) ){
            ds.up    = false
            ds.right = true
            ds.down  = false
            ds.left  = false
        }
        return ds
    }


    private val transPaint = Paint().apply {
        style = Paint.Style.FILL // 塗りつぶしスタイルを設定
        color = 0xFF000000.toInt() // 色を設定（この例では緑）
    }


    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {

        //canvas.drawRect(rect,pushPaint)
        var dstate = 0
        var startDir = 0.0f
        if (ds.left) {
            dstate = dstate or 0x01
        }
        if (ds.right) {
            dstate = dstate or 0x02
        }
        if (ds.up) {
            dstate = dstate or 0x04
        }
        if (ds.down) {
            dstate = dstate or 0x08
        }

        if( dstate == 0 ) return

        if( dstate == 0x02 ){ startDir = -22.5f }
        if( dstate == 0x0A ){ startDir = -22.5f + 45.0f  }
        if( dstate == 0x08 ){ startDir = -22.5f + (45.0f*2) }
        if( dstate == 0x09 ){ startDir = -22.5f + (45.0f*3) }
        if( dstate == 0x01 ){ startDir = -22.5f + (45.0f*4) }
        if( dstate == 0x05 ){ startDir = -22.5f + (45.0f*5) }
        if( dstate == 0x04 ){ startDir = -22.5f + (45.0f*6) }
        if( dstate == 0x06 ){ startDir = -22.5f + (45.0f*7) }

        canvas.save()
        canvas.clipPath(path)
        canvas.drawArc(drect, startDir, 45.0f, true, pushPaint)
        canvas.restore()
    }

    override fun Off(){
        ds.right = false
        ds.left = false
        ds.down = false
        ds.up = false
        super.Off()
    }
}



internal class AnalogPad(
    private val width: Int,
    private val text: String,
    private val textsize: Int,
) : PadButton() {
    private val paint = Paint()
    fun getXvalue(posx: Int): Int {
        var xv = (posx - rect.centerX()) / (width * scale / 2) * 128 + 128
        if (xv > 255) xv = 255f
        if (xv < 0) xv = 0f
        return xv.toInt()
    }

    fun getYvalue(posy: Int): Int {
        var yv = (posy - rect.centerY()) / (width * scale / 2) * 128 + 128
        if (yv > 255) yv = 255f
        if (yv < 0) yv = 0f
        return yv.toInt()
    }

    fun draw(
        canvas: Canvas,
        sx: Int,
        sy: Int,
        nomal_back: Paint?,
        active_back: Paint?,
        front: Paint?,
    ) {
        super.draw(canvas, nomal_back, active_back, front)
        // canvas.drawCircle(rect.centerX(), rect.centerY(), width * this.scale, back);
        // front.setTextSize(textsize);
        // front.setTextAlign(Paint.Align.CENTER);
        // canvas.drawText(text, rect.centerX() , rect.centerY() , front);
        //canvas.drawRect(rect,pushPaint)

        val dx = (sx - 128.0) / 128.0 * (width * scale / 2)
        val dy = (sy - 128.0) / 128.0 * (width * scale / 2)
        canvas.drawCircle(rect.centerX() + dx.toInt(),
            rect.centerY() + dy.toInt(),
            width * scale / 2,
            paint)

    }

    init {
        paint.setARGB(0x80, 0x80, 0x80, 0x80)
    }
}

class DraggableBitmap(
    val bitmap: Bitmap,
    var x: Float,
    var y: Float,
    var centerX: Float,
    var centerY: Float,
    var scale: Float,
){
    val width : Float
        get(){return bitmap.width.toFloat() }
    val height: Float
        get(){ return bitmap.height.toFloat() }

    var matrix : Matrix = Matrix()

    fun updateMatrix() : Matrix{

        matrix.reset()

        matrix.postTranslate(-centerX,-centerY)
        matrix.postScale(scale, scale)
        matrix.postTranslate(centerX,centerY)
        matrix.postTranslate(x,y)

        return matrix
    }

}

class YabausePad : View, OnTouchListener {
    interface OnPadListener {
        fun onPad(event: PadEvent?): Boolean
    }

    private var isDragging = false
    private lateinit var buttons: Array<PadButton?>
    private var listener: OnPadListener? = null
    private var active: HashMap<Int, Int>? = null
    private var vibrator: Vibrator? = null

    // private DisplayMetrics metrics = null;
    var width_ = 0
    var height_ = 0
    var bitmap_pad_left: DraggableBitmap? = null
    var bitmap_pad_right: DraggableBitmap? = null
    var bitmap_pad_top_left: DraggableBitmap? = null
    var bitmap_pad_top_right: DraggableBitmap? = null
    var bitmap_pad_middle: DraggableBitmap? = null

    var bitmap_pad_left_h: DraggableBitmap? = null
    var bitmap_pad_right_h: DraggableBitmap? = null
    var bitmap_pad_top_left_h: DraggableBitmap? = null
    var bitmap_pad_top_right_h: DraggableBitmap? = null
    var bitmap_pad_middle_h: DraggableBitmap? = null

    var isShow : Boolean = true

    private val mPaint = Paint()
    private val paint = Paint()
    private val apaint = Paint()
    private val tpaint = Paint()
    var scale = 1.0f
    var leftYPosition = 0.0f
    var topLeftYPosition = 0.0f
    var centerYPosition = 0.0f
    var rightYPosition = 0.0f
    var topRightYPosition = 0.0f
    var visualFeedback = true
    var forceFeedback = true
    var basewidth = 1920.0f
    var baseheight = 1080.0f
    private var wscale = 0f
    private var hscale = 0f
    var padTestestMode: Boolean = false
    var statusString: String? = null
        private set
    private var _analog_pad: AnalogPad? = null
    lateinit private var _dpad : Dpad
    private var _axi_x = 128
    private var _axi_y = 128
    private var _pad_mode = 0
    var trans = 1.0f

    var bitmaps: MutableList<DraggableBitmap?>? = null
    private var draggingBitmap: DraggableBitmap? = null

    fun setPadMode(mode: Int) {
        _pad_mode = mode
        invalidate()
    }

    private fun findTouchedBitmap(x: Float, y: Float): DraggableBitmap? {
        return bitmaps?.find { bitmap ->
            x >= bitmap!!.x && x <= bitmap.x + bitmap.bitmap.width &&
                    y >= bitmap.y && y <= bitmap.y + bitmap.bitmap.height
        }
    }

    private var lastX = 0f
    private var lastY = 0f


    fun onTouchDragging(v: View, event: MotionEvent) : Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                lastX = event.x
                lastY = event.y
                draggingBitmap = findTouchedBitmap(lastX, lastY)
            }

            MotionEvent.ACTION_MOVE -> {
                if( draggingBitmap == null ){
                    lastX = event.x
                    lastY = event.y
                    draggingBitmap = findTouchedBitmap(lastX, lastY)
                }
                draggingBitmap?.let {
                    val x = event.x
                    val y = event.y
                    val deltaX = x - lastX
                    val deltaY = y - lastY

                    it.x += deltaX
                    it.y += deltaY
                    it.updateMatrix()

                    lastX = x
                    lastY = y

                    invalidate()
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                isDragging = false
                draggingBitmap = null
                updateButtonPos()
            }
        }
        return true
    }


    fun show(b: Boolean ) {
        isShow = b
        if (b == false) {
            bitmap_pad_top_left = null
            bitmap_pad_top_right = null
            bitmap_pad_middle = null
            bitmap_pad_left = null
            bitmap_pad_right = null

            bitmap_pad_top_left_h = null
            bitmap_pad_top_right_h = null
            bitmap_pad_middle_h = null
            bitmap_pad_left_h = null
            bitmap_pad_right_h = null

            bitmaps?.clear()
        } else {
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
            val options = BitmapFactory.Options().apply {
                inScaled = false
            }
            val bpl = BitmapFactory.decodeResource(resources, R.drawable.pad_l_new,options)
            bitmap_pad_left = DraggableBitmap(
                bpl,
                sharedPref.getFloat("bitmap_pad_left_x", -1f),
                sharedPref.getFloat("bitmap_pad_left_y", -1f),
                -1F,
                -1F,
                1.0f
            )
            bitmap_pad_left_h = DraggableBitmap(
                bpl,
                sharedPref.getFloat("bitmap_pad_left_x_h", -1f),
                sharedPref.getFloat("bitmap_pad_left_y_h", -1f),
                -1F,
                -1F,
                1.0f
            )

            val bpr = BitmapFactory.decodeResource(resources, R.drawable.pad_r_new,options)
            bitmap_pad_right = DraggableBitmap(
                bpr,
                sharedPref.getFloat("bitmap_pad_right_x", -1f),
                sharedPref.getFloat("bitmap_pad_right_y", -1f),
                -1f,
                -1f,
                1.0f
            )
            bitmap_pad_right_h = DraggableBitmap(
                bpr,
                sharedPref.getFloat("bitmap_pad_right_x_h", -1f),
                sharedPref.getFloat("bitmap_pad_right_y_h", -1f),
                -1f,
                -1f,
                1.0f
            )

            val ptl = BitmapFactory.decodeResource(resources, R.drawable.pad_top_l_new,options)
            bitmap_pad_top_left = DraggableBitmap(ptl,
                sharedPref.getFloat("bitmap_pad_top_left_x", -1f),
                sharedPref.getFloat("bitmap_pad_top_left_y", -1f),
                ptl.width /2F,
                ptl.height /2F,
                1.0f
            )
            bitmap_pad_top_left_h = DraggableBitmap(ptl,
                sharedPref.getFloat("bitmap_pad_top_left_x_h", -1f),
                sharedPref.getFloat("bitmap_pad_top_left_y_h", -1f),
                ptl.width /2F,
                ptl.height /2F,
                1.0f
            )

            val ptr = BitmapFactory.decodeResource(resources, R.drawable.pad_top_r_new,options)
            bitmap_pad_top_right = DraggableBitmap(ptr,
                sharedPref.getFloat("bitmap_pad_top_right_x", -1f),
                sharedPref.getFloat("bitmap_pad_top_right_y", -1f),
                ptr.width /2F,
                ptr.height /2F,
                1.0f
            )
            bitmap_pad_top_right_h = DraggableBitmap(ptr,
                sharedPref.getFloat("bitmap_pad_top_right_x_h", -1f),
                sharedPref.getFloat("bitmap_pad_top_right_y_h", -1f),
                ptr.width /2F,
                ptr.height /2F,
                1.0f
            )

            val btn = BitmapFactory.decodeResource(resources, R.drawable.pad_m_new,options)
            bitmap_pad_middle = DraggableBitmap(btn,
                sharedPref.getFloat("bitmap_pad_middle_x", -1f),
                sharedPref.getFloat("bitmap_pad_middle_y", -1f),
                btn.width /2F,
                btn.height /2F,
                1.0f
            )
            bitmap_pad_middle_h = DraggableBitmap(btn,
                sharedPref.getFloat("bitmap_pad_middle_x_h", -1f),
                sharedPref.getFloat("bitmap_pad_middle_y_h", -1f),
                btn.width /2F,
                btn.height /2F,
                1.0f
            )

            bitmaps = mutableListOf(
                bitmap_pad_middle,
                bitmap_pad_top_left,
                bitmap_pad_top_right,
                bitmap_pad_left,
                bitmap_pad_right,
            )

        }
        invalidate()
    }

    fun saveCurrentPositionState(){
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        val editor = sharedPref.edit()

        if( bitmap_pad_left != null ) {
            editor.putFloat("bitmap_pad_left_x", bitmap_pad_left!!.x)
            editor.putFloat("bitmap_pad_left_y", bitmap_pad_left!!.y)
        }
        if( bitmap_pad_right != null ) {
            editor.putFloat("bitmap_pad_right_x", bitmap_pad_right!!.x)
            editor.putFloat("bitmap_pad_right_y", bitmap_pad_right!!.y)
        }
        if( bitmap_pad_top_left != null ) {
            editor.putFloat("bitmap_pad_top_left_x", bitmap_pad_top_left!!.x)
            editor.putFloat("bitmap_pad_top_left_y", bitmap_pad_top_left!!.y)
        }
        if( bitmap_pad_top_right != null ) {
            editor.putFloat("bitmap_pad_top_right_x", bitmap_pad_top_right!!.x)
            editor.putFloat("bitmap_pad_top_right_y", bitmap_pad_top_right!!.y)
        }
        if( bitmap_pad_middle != null ) {
            editor.putFloat("bitmap_pad_middle_x", bitmap_pad_middle!!.x)
            editor.putFloat("bitmap_pad_middle_y", bitmap_pad_middle!!.y)
        }

        if( bitmap_pad_left_h != null ) {
            editor.putFloat("bitmap_pad_left_x_h", bitmap_pad_left_h!!.x)
            editor.putFloat("bitmap_pad_left_y_h", bitmap_pad_left_h!!.y)
        }
        if( bitmap_pad_right_h != null ) {
            editor.putFloat("bitmap_pad_right_x_h", bitmap_pad_right_h!!.x)
            editor.putFloat("bitmap_pad_right_y_h", bitmap_pad_right_h!!.y)
        }
        if( bitmap_pad_top_left_h != null ) {
            editor.putFloat("bitmap_pad_top_left_x_h", bitmap_pad_top_left_h!!.x)
            editor.putFloat("bitmap_pad_top_left_y_h", bitmap_pad_top_left_h!!.y)
        }
        if( bitmap_pad_top_right_h != null ) {
            editor.putFloat("bitmap_pad_top_right_x_h", bitmap_pad_top_right_h!!.x)
            editor.putFloat("bitmap_pad_top_right_y_h", bitmap_pad_top_right_h!!.y)
        }
        if( bitmap_pad_middle_h != null ) {
            editor.putFloat("bitmap_pad_middle_x_h", bitmap_pad_middle_h!!.x)
            editor.putFloat("bitmap_pad_middle_y_h", bitmap_pad_middle_h!!.y)
        }

        editor.commit()
    }

    constructor(context: Context?) : super(context) {
        init()
    }

    constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {
        init()
    }

    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(context,
        attrs,
        defStyle) {
        init()
    }

    fun setTestmode(test: Boolean) {
        padTestestMode = test
    }

    fun getPreferences(){
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        scale = sharedPref.getFloat("pref_pad_scale", 0.75f)
        leftYPosition = sharedPref.getFloat("pref_pad_pos", 0.1f)
        centerYPosition = sharedPref.getFloat("pref_pad_center_pos", 0.1f)
        rightYPosition = sharedPref.getFloat("pref_pad_right_pos", 0.1f)
        topLeftYPosition = sharedPref.getFloat("pref_pad_top_left_pos", 1.2f)
        topRightYPosition = sharedPref.getFloat("pref_pad_top_right_pos", 1.2f)
        trans = sharedPref.getFloat("pref_pad_trans", 0.7f)
        visualFeedback = sharedPref.getBoolean("pref_visual_feedback", true)
        forceFeedback = sharedPref.getBoolean("pref_force_feedback", true)

        bitmap_pad_left?.x = sharedPref.getFloat("bitmap_pad_left_x", -1F)
        bitmap_pad_left?.y = sharedPref.getFloat("bitmap_pad_left_y", -1F)
        bitmap_pad_right?.x = sharedPref.getFloat("bitmap_pad_right_x", -1F)
        bitmap_pad_right?.y = sharedPref.getFloat("bitmap_pad_right_y", -1F)
        bitmap_pad_top_right?.x = sharedPref.getFloat("bitmap_pad_top_right_x", -1F)
        bitmap_pad_top_right?.y = sharedPref.getFloat("bitmap_pad_top_right_y", -1F)
        bitmap_pad_top_left?.x = sharedPref.getFloat("bitmap_pad_top_left_x", -1F)
        bitmap_pad_top_left?.y = sharedPref.getFloat("bitmap_pad_top_left_y", -1F)
        bitmap_pad_middle?.x = sharedPref.getFloat("bitmap_pad_middle_x", -1F)
        bitmap_pad_middle?.y = sharedPref.getFloat("bitmap_pad_middle_y", -1F)

        bitmap_pad_left_h?.x = sharedPref.getFloat("bitmap_pad_left_x_h", -1F)
        bitmap_pad_left_h?.y = sharedPref.getFloat("bitmap_pad_left_y_h", -1F)
        bitmap_pad_right_h?.x = sharedPref.getFloat("bitmap_pad_right_x_h", -1F)
        bitmap_pad_right_h?.y = sharedPref.getFloat("bitmap_pad_right_y_h", -1F)
        bitmap_pad_top_right_h?.x = sharedPref.getFloat("bitmap_pad_top_right_x_h", -1F)
        bitmap_pad_top_right_h?.y = sharedPref.getFloat("bitmap_pad_top_right_y_h", -1F)
        bitmap_pad_top_left_h?.x = sharedPref.getFloat("bitmap_pad_top_left_x_h", -1F)
        bitmap_pad_top_left_h?.y = sharedPref.getFloat("bitmap_pad_top_left_y_h", -1F)
        bitmap_pad_middle_h?.x = sharedPref.getFloat("bitmap_pad_middle_x_h", -1F)
        bitmap_pad_middle_h?.y = sharedPref.getFloat("bitmap_pad_middle_y_h", -1F)

    }

    fun updateScale() {
        // setPadScale( width_, height_ );
        getPreferences()
        requestLayout()
        this.invalidate()
    }

    private fun init() {
        setOnTouchListener(this)
        isLongClickable = true
        setOnLongClickListener {
            if( this.padTestestMode ) {
                isDragging = true
                var btnindex = 4
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn() ) {
                        buttons[btnindex]!!.Off()
                        invalidate()
                    }
                    btnindex++
                }
                _analog_pad!!.Off()
                _dpad.Off()
                preDstate = 0
            }
            true
        }
        getPreferences()
        buttons = arrayOfNulls(PadEvent.BUTTON_LAST)
        vibrator = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val vibratorManager =
                context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as VibratorManager
            vibratorManager.defaultVibrator
        } else {
            @Suppress("DEPRECATION")
            context.getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
        }
        if (vibrator == null) {
            Log.e("Vibration", "Vibrator service is not available")
        } else if (vibrator?.hasVibrator() != true) {
            Log.e("Vibration", "This device does not support vibration")
            vibrator = null
        }
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER] = DPadButton()
        buttons[PadEvent.BUTTON_LEFT_TRIGGER] = DPadButton()
        buttons[PadEvent.BUTTON_START] = StartButton()
        buttons[PadEvent.BUTTON_A] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_B] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_C] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_X] = ActionButton(72, "", 25)
        buttons[PadEvent.BUTTON_Y] = ActionButton(72, "", 25)
        buttons[PadEvent.BUTTON_Z] = ActionButton(72, "", 25)
        _analog_pad = AnalogPad(256, "", 40)
        _dpad = Dpad(256, 60)
        active = HashMap()
    }

    override fun onAttachedToWindow() {
        paint.setARGB(0xFF, 0, 0, 0xFF)
        apaint.setARGB(0xFF, 0xFF, 0x00, 0x00)
        tpaint.setARGB(0x80, 0xFF, 0xFF, 0xFF)
        // bitmap_pad_left = BitmapFactory.decodeResource(getResources(), R.drawable.pad_l);
        // bitmap_pad_right= BitmapFactory.decodeResource(getResources(), R.drawable.pad_r);
        mPaint.isAntiAlias = true
        mPaint.isFilterBitmap = true
        mPaint.isDither = true
        super.onAttachedToWindow()
    }

    public override fun onDraw(canvas: Canvas) {
        if (!isShow) {
            return
        }

        mPaint.alpha = (255.0f * trans).toInt()
        if( bitmaps != null ){
            for (item in bitmaps!!.reversed()) {
                canvas.drawBitmap(item!!.bitmap, item.matrix, mPaint)
            }
        }

        canvas.setMatrix(null)
        if( !visualFeedback) return
        if( isDragging ) return

        canvas.save();
        buttons[PadEvent.BUTTON_LEFT_TRIGGER]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_A]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_B]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_C]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_X]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_Y]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_Z]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER]?.draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_START]?.draw(canvas, paint, apaint, tpaint);

        if (_pad_mode == PadManager.MODE_ANALOG) {
            _analog_pad!!.draw(canvas, _axi_x, _axi_y, paint, apaint, tpaint)
        }else{
            _dpad.draw(canvas, paint, apaint, tpaint)
        }

    }

    fun setOnPadListener(listener: OnPadListener?) {
        this.listener = listener
    }

    var preDstate = 0

    fun viberate(){

        if( !forceFeedback) return

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            vibrator?.vibrate(
                VibrationEffect.createOneShot(
                    16,
                    VibrationEffect.DEFAULT_AMPLITUDE
                )
            )
        }
    }

    fun actionDpadState(dpadState : DpadState ){
        var dstate = 0
        if (!padTestestMode) {
            if (dpadState.left) {
                YabauseRunnable.press(PadEvent.BUTTON_LEFT, 0)
            } else {
                YabauseRunnable.release(PadEvent.BUTTON_LEFT, 0)
            }
            if (dpadState.right) {
                YabauseRunnable.press(PadEvent.BUTTON_RIGHT, 0)
            } else {
                YabauseRunnable.release(PadEvent.BUTTON_RIGHT, 0)
            }
            if (dpadState.up) {
                YabauseRunnable.press(PadEvent.BUTTON_UP, 0)
            } else {
                YabauseRunnable.release(PadEvent.BUTTON_UP, 0)
            }
            if (dpadState.down) {
                YabauseRunnable.press(PadEvent.BUTTON_DOWN, 0)
            } else {
                YabauseRunnable.release(PadEvent.BUTTON_DOWN, 0)
            }
        }

        if (dpadState.left) {
            dstate = dstate or 0x01
        }
        if (dpadState.right) {
            dstate = dstate or 0x02
        }
        if (dpadState.up) {
            dstate = dstate or 0x04
        }
        if (dpadState.down) {
            dstate = dstate or 0x08
        }

        if( preDstate != dstate){
            if( dstate != 0 ){
                viberate()
            }
            preDstate = dstate
        }

    }



    private fun updateDPad(hittest: RectF, posx: Int, posy: Int, pointerId: Int) {
        if (_dpad.intersects(hittest)) {
            _dpad.On(pointerId)
            invalidate()
            val dpadState = _dpad.getState(posx,posy)
            actionDpadState(dpadState)
        } else if (_dpad.isOn(pointerId)) {
            val dpadState = _dpad.getState(posx,posy)
            invalidate()
            actionDpadState(dpadState)
        }
    }

    private fun releaseDPad(pointerId: Int) {
        if (_dpad.isOn(pointerId)) {
            _dpad.Off()
            if (!padTestestMode) {
                YabauseRunnable.release(PadEvent.BUTTON_LEFT, 0)
                YabauseRunnable.release(PadEvent.BUTTON_RIGHT, 0)
                YabauseRunnable.release(PadEvent.BUTTON_UP, 0)
                YabauseRunnable.release(PadEvent.BUTTON_DOWN, 0)
                preDstate = 0
            }
            invalidate()
        }
    }


    private fun updatePad(hittest: RectF, posx: Int, posy: Int, pointerId: Int) {
        if (_analog_pad!!.intersects(hittest)) {
            _analog_pad!!.On(pointerId)
            _axi_x = _analog_pad!!.getXvalue(posx)
            _axi_y = _analog_pad!!.getYvalue(posy)
            invalidate()
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
        } else if (_analog_pad!!.isOn(pointerId)) {
            _axi_x = _analog_pad!!.getXvalue(posx)
            _axi_y = _analog_pad!!.getYvalue(posy)
            invalidate()
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
        }
    }

    private fun releasePad(pointerId: Int) {
        if (_analog_pad!!.isOn(pointerId)) {
            _analog_pad!!.Off()
            _axi_x = 128
            _axi_y = 128
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
            invalidate()
        }
    }



    override fun onTouch(v: View, event: MotionEvent): Boolean {

        if( isDragging ){
            return onTouchDragging( v, event )
        }

        val action = event.actionMasked
        val touchCount = event.pointerCount
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)
        val posx = event.getX(pointerIndex).toInt()
        val posy = event.getY(pointerIndex).toInt()
        val hitsize = 15.0f * wscale * scale
        val hittest = RectF((posx - hitsize).toFloat(),
            (posy - hitsize).toFloat(),
            (posx + hitsize).toFloat(),
            (posy + hitsize).toFloat())
        when (action) {
            MotionEvent.ACTION_DOWN -> {
                var btnindex = 4
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.intersects(hittest)) {
                        if( buttons[btnindex]!!.isOn(pointerId) != true ){
                            viberate()
                            invalidate()
                        }
                        buttons[btnindex]!!.On(pointerId)
                    }
                    btnindex++
                }
                if (_pad_mode == PadManager.MODE_ANALOG) {
                    updatePad(hittest, posx, posy, pointerId)
                }else{
                    updateDPad(hittest, posx, posy, pointerId)
                }
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                var btnindex = 4
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.intersects(hittest)) {
                        if( buttons[btnindex]!!.isOn(pointerId) != true ){
                            viberate()
                            invalidate()
                        }
                        buttons[btnindex]!!.On(pointerId)
                    }
                    btnindex++
                }
                if (_pad_mode == PadManager.MODE_ANALOG) {
                    updatePad(hittest, posx, posy, pointerId)
                }else{
                    updateDPad(hittest, posx, posy, pointerId)
                }
            }
            MotionEvent.ACTION_POINTER_UP -> {
                var btnindex = 4
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn() && buttons[btnindex]!!.pointId == pointerId) {
                        buttons[btnindex]!!.Off()
                        invalidate()
                    }
                    btnindex++
                }
                if (_pad_mode == PadManager.MODE_ANALOG) {
                    releasePad(pointerId)
                }else{
                    releaseDPad(pointerId)
                }
            }
            MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_UP -> {
                var btnindex = 4
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn() && buttons[btnindex]!!.pointId == pointerId) {
                        buttons[btnindex]!!.Off()
                        invalidate()
                    }
                    btnindex++
                }
                if (_pad_mode == PadManager.MODE_ANALOG) {
                    releasePad(pointerId)
                }else{
                    releaseDPad(pointerId)
                }
            }
            MotionEvent.ACTION_MOVE -> {
                var index = 0
                while (index < touchCount) {
                    val eventID2 = event.getPointerId(index)
                    val x2 = event.getX(index)
                    val y2 = event.getY(index)
                    val hittest2 = RectF((x2 - hitsize).toFloat(),
                        (y2 - hitsize).toFloat(),
                        (x2 + hitsize).toFloat(),
                        (y2 + hitsize).toFloat())
                    var btnindex = PadEvent.BUTTON_RIGHT_TRIGGER
                    while (btnindex < PadEvent.BUTTON_LAST) {
                        if (eventID2 == buttons[btnindex]!!.pointId) {
                            if (buttons[btnindex]!!.intersects(hittest2) == false) {
                                buttons[btnindex]!!.Off()
                                invalidate()
                            }
                        } else if (buttons[btnindex]!!.intersects(hittest2)) {
                            buttons[btnindex]!!.On(eventID2)
                        }
                        btnindex++
                    }
                    if (_pad_mode == 1) {
                        updatePad(hittest2, x2.toInt(), y2.toInt(), eventID2)
                    }else{
                        updateDPad(hittest2, x2.toInt(), y2.toInt(), eventID2)
                    }
                    index++
                }
            }
        }
        if (!padTestestMode) {
            if (_pad_mode == 0) {
                for (btnindex in 4 until PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn()) {
                        YabauseRunnable.press(btnindex, 0)
                    } else {
                        YabauseRunnable.release(btnindex, 0)
                    }
                }
            } else {
                for (btnindex in PadEvent.BUTTON_RIGHT_TRIGGER until PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn()) {
                        YabauseRunnable.press(btnindex, 0)
                        if (btnindex == PadEvent.BUTTON_RIGHT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_RTRIGGER, 0, 255)
                        }
                        if (btnindex == PadEvent.BUTTON_LEFT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_LTRIGGER, 0, 255)
                        }
                    } else {
                        YabauseRunnable.release(btnindex, 0)
                        if (btnindex == PadEvent.BUTTON_RIGHT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_RTRIGGER, 0, 0)
                        }
                        if (btnindex == PadEvent.BUTTON_LEFT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_LTRIGGER, 0, 0)
                        }
                    }
                }
            }
        }
        if (padTestestMode) {
            statusString = ""
            statusString += "START:"
            if (buttons[PadEvent.BUTTON_START]!!.isOn()) statusString += "ON " else statusString += "OFF "

            statusString += "\nDpad: "
            if (_dpad.ds.up){
                if (_dpad.ds.left){
                    statusString += "\u2196"
                }else if(_dpad.ds.right){
                    statusString += "\u2197"
                }else{
                    statusString += "\u2191"
                }
            }else if(_dpad.ds.down){
                if (_dpad.ds.left){
                    statusString += "\u2199"
                }else if(_dpad.ds.right){
                    statusString += "\u2198"
                }else{
                    statusString += "\u2193"
                }
            }else{
                if (_dpad.ds.left){
                    statusString += "\u2190"
                }else if(_dpad.ds.right) {
                    statusString += "\u2192"
                }
            }
            statusString += "\nA:"
            if (buttons[PadEvent.BUTTON_A]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "B:"
            if (buttons[PadEvent.BUTTON_B]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "C:"
            if (buttons[PadEvent.BUTTON_C]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nX:"
            if (buttons[PadEvent.BUTTON_X]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "Y:"
            if (buttons[PadEvent.BUTTON_Y]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "Z:"
            if (buttons[PadEvent.BUTTON_Z]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nLT:"
            if (buttons[PadEvent.BUTTON_LEFT_TRIGGER]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "RT:"
            if (buttons[PadEvent.BUTTON_RIGHT_TRIGGER]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nAX:"
            if (_analog_pad!!.isOn()) statusString += "ON $_axi_x" else statusString += "OFF $_axi_x"
            statusString += "AY:"
            if (_analog_pad!!.isOn()) statusString += "ON $_axi_y" else statusString += "OFF $_axi_y"
        }
        if (listener != null) {
            listener!!.onPad(null)
        }
        return false
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        if (!isShow) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            return
        }
        width_ = MeasureSpec.getSize(widthMeasureSpec)
        height_ = MeasureSpec.getSize(heightMeasureSpec)
        setPadScale(width_, height_)
    }

    val buttonCenterX = 347
    val buttonCenterY = 720
    val rectsize = 230

    fun updateButtonPos(){
        if( bitmaps == null ) return

        _analog_pad!!.updateRect(bitmaps!!.get(PAD_LEFT)!!.matrix,
            buttonCenterX-rectsize, buttonCenterY-rectsize,
            buttonCenterX + rectsize, buttonCenterY + rectsize)
        _analog_pad!!.updateScale(scale)

        _dpad.updateRect(bitmaps!!.get(PAD_LEFT)!!.matrix,
            buttonCenterX-rectsize, buttonCenterY -rectsize,
            buttonCenterX+rectsize, buttonCenterY +rectsize)
        _dpad.updateScale(scale)

        // buttons[PadEvent.BUTTON_START].updateRect(matrix_left,510,1013,510+182,1013+57);
        buttons[PadEvent.BUTTON_START]!!
            .updateRect(bitmaps!!.get(PAD_MIDDLE)!!.matrix, 0, 53, 185, 132)

        // Right Part
        buttons[PadEvent.BUTTON_A]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 59, 801, 59 + 213, 801 + 225)
        buttons[PadEvent.BUTTON_A]!!.updateScale(scale)
        buttons[PadEvent.BUTTON_B]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 268, 672, 268 + 229, 672 + 221)
        buttons[PadEvent.BUTTON_B]!!.updateScale(scale)
        buttons[PadEvent.BUTTON_C]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 507, 577, 507 + 224, 577 + 229)
        buttons[PadEvent.BUTTON_C]!!.updateScale(scale)
        buttons[PadEvent.BUTTON_X]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 15, 602, 15 + 149, 602 + 150)
        buttons[PadEvent.BUTTON_X]!!.updateScale(scale)
        buttons[PadEvent.BUTTON_Y]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 202, 481, 202 + 149, 481 + 148)
        buttons[PadEvent.BUTTON_Y]!!.updateScale(scale)
        buttons[PadEvent.BUTTON_Z]!!.updateRect(bitmaps!!.get(PAD_RIGHT)!!.matrix, 397, 409, 397 + 151, 409 + 152)
        buttons[PadEvent.BUTTON_Z]!!.updateScale(scale)

        buttons[PadEvent.BUTTON_LEFT_TRIGGER]!!.updateRect(bitmaps!!.get(PAD_TOP_LEFT)!!.matrix,
            57,
            48,
            57 + 379,
            48 + 100)
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER]!!.updateRect(bitmaps!!.get(PAD_TOP_RIGHT)!!.matrix,
            338,
            48,
            338 + 379,
            48 + 100)

    }

    val PAD_MIDDLE = 0
    val PAD_TOP_LEFT = 1
    val PAD_TOP_RIGHT = 2
    val PAD_LEFT = 3
    val PAD_RIGHT = 4

    fun setPadScale(width: Int, height: Int) {

        var dens = resources.displayMetrics.density
        val dir = context.getResources().getConfiguration().orientation

        val leftPadPosX = -120F
        var leftPadPosY = 500F

        val rightPadPosX = -60F
        var rightPadPosY = 80F

        val triggerX = -40F
        var triggerY = 1080f - 250f

        // 横画面
        if (width > height) {
            leftPadPosY = 0F
            rightPadPosY = 0F
            triggerY = 0F
            bitmaps = mutableListOf(
                bitmap_pad_middle_h,
                bitmap_pad_top_left_h,
                bitmap_pad_top_right_h,
                bitmap_pad_left_h,
                bitmap_pad_right_h,
            )
        }else{
            bitmaps = mutableListOf(
                bitmap_pad_middle,
                bitmap_pad_top_left,
                bitmap_pad_top_right,
                bitmap_pad_left,
                bitmap_pad_right,
            )
        }

        // midedle
        bitmaps?.get(PAD_MIDDLE)?.apply {
            if( x == -1F && y == -1F){
                x = (width / 2).toFloat() - (this.width / 2.0F)
                y = height - this.height
            }
            centerX = this.width / 2.0F
            centerY = this.height / 2.0F
            scale = this@YabausePad.scale
            updateMatrix()
        }

        // Top left
        bitmaps?.get(PAD_TOP_LEFT)?.apply{
            if( x == -1F && y == -1F){
                x = triggerX
                y = triggerY
            }
            centerX = this.width / 2.0F
            centerY = this.height / 2.0F
            scale = this@YabausePad.scale
            updateMatrix()
        }

        // Top right
        bitmaps?.get(PAD_TOP_RIGHT)?.apply{
            if( x == -1F && y == -1F){
                x = width.toFloat() - (this.width.toFloat()+triggerX)
                y = triggerY
            }
            centerX = this.width / 2.0F
            centerY = this.height / 2.0F
            scale = this@YabausePad.scale
            updateMatrix()
        }

        // bitmap_pad_left
        bitmaps?.get(PAD_LEFT)?.apply{
            if( x == -1F && y == -1F){
                x = leftPadPosX
                y = height - ((this.height.toFloat()) + leftPadPosY)
            }
            centerX = buttonCenterX.toFloat()
            centerY = buttonCenterY.toFloat()
            scale = this@YabausePad.scale
            updateMatrix()
        }

        // bitmap_pad_right
        bitmaps?.get(PAD_RIGHT)?.apply{
            if( x == -1F && y == -1F){
                x = width.toFloat() -( this.width.toFloat() + rightPadPosX)
                y = height -(this.height.toFloat()+rightPadPosY)
            }
            centerX = 315f
            centerY = 652f
            scale = this@YabausePad.scale
            updateMatrix()
        }
        updateButtonPos()
        setMeasuredDimension(width, height)
    }
}


