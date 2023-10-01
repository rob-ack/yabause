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

import android.app.AlertDialog
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.CompoundButton
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.preference.PreferenceManager
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.PadManager.Companion.padManager
import org.uoyabause.android.YabausePad.OnPadListener

class PadTestFragment : Fragment(), OnPadListener {
    var mPadView: YabausePad? = null
    var mSlide: SeekBar? = null
    var mLeftSlideY: SeekBar? = null
    var mCenterSlideY: SeekBar? = null
    var mRightSlideY: SeekBar? = null
    var mTransSlide: SeekBar? = null
    private var padm: PadManager? = null
    var tv: TextView? = null

    var mChkForceFeedback: CheckBox? = null
    var mChkVIsualFeedback: CheckBox? = null


    interface PadTestListener {
        fun onFinish()
        fun onCancel()
    }

    var listener_: PadTestListener? = null
    fun setListener(listner: PadTestListener?) {
        listener_ = listner
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val rootView = inflater.inflate(R.layout.padtest, container, false)
        padm = padManager
        padm!!.setTestMode(true)
        padm!!.loadSettings()
        mPadView = rootView.findViewById<View>(R.id.yabause_pad) as YabausePad
        mPadView!!.setTestmode(true)
        mPadView!!.setOnPadListener(this)
        mPadView!!.show(true)
        mSlide = rootView.findViewById<View>(R.id.button_scale) as SeekBar
        mSlide!!.progress = (mPadView!!.scale * 100.0f).toInt()
        mSlide!!.setOnSeekBarChangeListener(
            object : OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mPadView!!.scale = progress.toFloat() / 100.0f
                    mPadView!!.requestLayout()
                    mPadView!!.invalidate()
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {}
                override fun onStopTrackingTouch(seekBar: SeekBar) {}
            }
        )
/*
        mLeftSlideY = rootView.findViewById<View>(R.id.button_lypos) as SeekBar
        mLeftSlideY!!.progress = (mPadView!!.leftYPosition * 100.0f).toInt()
        mLeftSlideY!!.setOnSeekBarChangeListener(
            object : OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mPadView!!.leftYPosition = progress.toFloat() / 100.0f
                    mPadView!!.requestLayout()
                    mPadView!!.invalidate()
                }
                override fun onStartTrackingTouch(seekBar: SeekBar) {}
                override fun onStopTrackingTouch(seekBar: SeekBar) {}
            }
        )

        mCenterSlideY = rootView.findViewById<View>(R.id.button_cypos) as SeekBar
        mCenterSlideY!!.progress = (mPadView!!.centerYPosition * 100.0f).toInt()
        mCenterSlideY!!.setOnSeekBarChangeListener(
            object : OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mPadView!!.centerYPosition = progress.toFloat() / 100.0f
                    mPadView!!.requestLayout()
                    mPadView!!.invalidate()
                }
                override fun onStartTrackingTouch(seekBar: SeekBar) {}
                override fun onStopTrackingTouch(seekBar: SeekBar) {}
            }
        )


        mRightSlideY = rootView.findViewById<View>(R.id.button_rypos) as SeekBar
        mRightSlideY!!.progress = (mPadView!!.rightYPosition * 100.0f).toInt()
        mRightSlideY!!.setOnSeekBarChangeListener(
            object : OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mPadView!!.rightYPosition = progress.toFloat() / 100.0f
                    mPadView!!.requestLayout()
                    mPadView!!.invalidate()
                }
                override fun onStartTrackingTouch(seekBar: SeekBar) {}
                override fun onStopTrackingTouch(seekBar: SeekBar) {}
            }
        )
*/

        mTransSlide = rootView.findViewById<View>(R.id.button_transparent) as SeekBar
        mTransSlide!!.progress = (mPadView!!.trans * 100.0f).toInt()
        mTransSlide!!.setOnSeekBarChangeListener(
            object : OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mPadView!!.trans = progress.toFloat() / 100.0f
                    mPadView!!.invalidate()
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {}
                override fun onStopTrackingTouch(seekBar: SeekBar) {}
            }
        )

        mChkForceFeedback = rootView.findViewById<CheckBox>(R.id.cb_forcefeedback)
        mChkForceFeedback?.setOnCheckedChangeListener( object : CompoundButton.OnCheckedChangeListener{
            override fun onCheckedChanged(buttonView: CompoundButton?, isChecked: Boolean) {
                mPadView!!.forceFeedback = isChecked
            }
        })
        mChkForceFeedback?.isChecked = mPadView!!.forceFeedback

        mChkVIsualFeedback= rootView.findViewById<CheckBox>(R.id.cb_visual_feedback)
        mChkVIsualFeedback?.setOnCheckedChangeListener( object : CompoundButton.OnCheckedChangeListener{
            override fun onCheckedChanged(buttonView: CompoundButton?, isChecked: Boolean) {
                mPadView!!.visualFeedback = isChecked
            }
        })
        mChkVIsualFeedback?.isChecked = mPadView!!.visualFeedback

        tv = rootView.findViewById<View>(R.id.text_status) as TextView
        return rootView
    }

    fun onBackPressed() {
        val alert = AlertDialog.Builder(activity)
        alert.setTitle("")
        alert.setMessage(R.string.do_you_want_to_save_this_setting)
        alert.setPositiveButton(R.string.yes) { _, _ ->
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(
                requireActivity())
            val editor = sharedPref.edit()
            var value = mSlide!!.progress.toFloat() / 100.0f
            editor.putFloat("pref_pad_scale", value)
/*
            value = mLeftSlideY!!.progress.toFloat() / 100.0f
            editor.putFloat("pref_pad_pos", value)

            value = mRightSlideY!!.progress.toFloat() / 100.0f
            editor.putFloat("pref_pad_right_pos", value)
*/
            value = mTransSlide!!.progress.toFloat() / 100.0f
            editor.putFloat("pref_pad_trans", value)

            editor.putBoolean("pref_force_feedback", mChkForceFeedback!!.isChecked())
            editor.putBoolean("pref_visual_feedback", mChkVIsualFeedback!!.isChecked())
            editor.commit()

            if (listener_ != null) listener_!!.onFinish()
        }
        alert.setNegativeButton(R.string.no) { _, _ -> if (listener_ != null) listener_!!.onCancel() }
        alert.show()
    }

    override fun onPad(event: PadEvent?): Boolean {
        // TextView tv = (TextView)findViewById(R.id.text_status);
        tv!!.text = mPadView!!.statusString
        tv!!.invalidate()
        return true
    }

    companion object {
        const val TAG = "PadTestFragment"
        fun newInstance(): PadTestFragment {
            val fragment = PadTestFragment()
            val args = Bundle()
            fragment.arguments = args
            return fragment
        }
    }
}

