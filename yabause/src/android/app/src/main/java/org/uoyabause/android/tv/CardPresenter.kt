/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
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
package org.uoyabause.android.tv

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.drawable.Drawable
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import androidx.leanback.widget.ImageCardView
import androidx.leanback.widget.Presenter
import com.bumptech.glide.Glide
import com.bumptech.glide.request.target.CustomTarget
import com.bumptech.glide.request.transition.Transition
import com.google.firebase.analytics.FirebaseAnalytics
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.GameInfo
import java.io.File

/*
 * A CardPresenter is used to generate Views and bind Objects to them on demand. 
 * It contains an Image CardView
 */
class CardPresenter : Presenter() {
    private var mDefaultCardImage: Drawable? = null

    override fun onCreateViewHolder(parent: ViewGroup): ViewHolder {
        sDefaultBackgroundColor = parent.resources.getColor(R.color.default_background)
        sSelectedBackgroundColor = parent.resources.getColor(R.color.selected_background)
        mDefaultCardImage = parent.resources.getDrawable(R.drawable.missing)

        val cardView: ImageCardView = object : ImageCardView(parent.context) {
            override fun setSelected(selected: Boolean) {
                updateCardBackgroundColor(this, selected)
                super.setSelected(selected)
            }
        }

        cardView.isFocusable = true
        cardView.isFocusableInTouchMode = true
        updateCardBackgroundColor(cardView, false)
        return ViewHolder(cardView)
    }

    override fun onBindViewHolder(viewHolder: ViewHolder, item: Any?) {
        val game = item as GameInfo?
        val cardView = viewHolder.view as ImageCardView
        cardView.titleText = game!!.game_title
        var rate = ""
        for (i in 0 until game.rating) {
            rate += "★"
        }
        if (game.device_infomation == "CD-1/1") {
        } else {
            rate += " " + game.device_infomation
        }

        cardView.contentText = rate
        cardView.setMainImageDimensions(CARD_WIDTH, CARD_HEIGHT)
        val activity = viewHolder.view.context as Activity
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 && activity.isDestroyed) {
            return
        }
        if (game.image_url != "") {
            if (game.image_url!!.startsWith("http")) {
                Glide.with(viewHolder.view.context)
                    .asBitmap()
                    .load(game.image_url) //.centerCrop()
                    //.centerInside()
                    .placeholder(mDefaultCardImage) //.apply(new RequestOptions().transforms(new CenterCrop() )
                    //       .error(mDefaultCardImage))
                    //.into(cardView.getMainImageView());
                    .into(object : CustomTarget<Bitmap>() {

                        override fun onResourceReady(
                            resource: Bitmap,
                            transition: Transition<in Bitmap>?,
                        ) {
                            val bmp = resource as Bitmap
                            val w = bmp.width
                            val h = bmp.height

                            if (w < h) {
                                cardView.setMainImageDimensions(
                                    (CARD_HEIGHT * w.toFloat() / h.toFloat()).toInt(),
                                    CARD_HEIGHT
                                )
                            } else {
                                cardView.setMainImageDimensions(
                                    CARD_WIDTH,
                                    (CARD_WIDTH * h.toFloat() / w.toFloat()).toInt()
                                )
                            }
                            cardView.mainImageView!!.setImageBitmap(bmp)
                        }

                        /*
                        override fun onResourceReady(resource: Any, transition: Transition<*>?) {
                            val bmp = resource as Bitmap
                            val w = bmp.width
                            val h = bmp.height

                            if (w < h) {
                                cardView.setMainImageDimensions(
                                    (CARD_HEIGHT * w.toFloat() / h.toFloat()).toInt(),
                                    CARD_HEIGHT
                                )
                            } else {
                                cardView.setMainImageDimensions(
                                    CARD_WIDTH,
                                    (CARD_WIDTH * h.toFloat() / w.toFloat()).toInt()
                                )
                            }

                            cardView.mainImageView!!.setImageBitmap(bmp)
                        }
                        */

                        override fun onLoadCleared(placeholder: Drawable?) {
                        }

                        override fun onLoadFailed(placeholder: Drawable?) {
                            val mFirebaseAnalytics =
                                FirebaseAnalytics.getInstance(viewHolder.view.context)
                            val bundle = Bundle()
                            bundle.putString(FirebaseAnalytics.Param.ITEM_ID, game.product_number)
                            bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, game.image_url)
                            mFirebaseAnalytics.logEvent(
                                "yab_fail_load_image", bundle
                            )
                        }


                    })
            } else {
                Glide.with(viewHolder.view.context)
                    .load(File(game.image_url)) //.centerCrop()
                    .placeholder(mDefaultCardImage) //.apply(  new RequestOptions().transforms(new CenterCrop() ).error(mDefaultCardImage)  )
                    .into(cardView.mainImageView!!)
            }
        } else {
            cardView.mainImageView!!.setImageDrawable(mDefaultCardImage)
        }
    }

    override fun onUnbindViewHolder(viewHolder: ViewHolder) {
        val cardView = viewHolder.view as ImageCardView
        // Remove references to images so that the garbage collector can free up memory
        cardView.badgeImage = null
        cardView.mainImage = null
    }

    companion object {
        private const val TAG = "CardPresenter"

        private const val CARD_WIDTH = 320
        private const val CARD_HEIGHT = (CARD_WIDTH * 12.5 / 14.0).toInt()
        private var sSelectedBackgroundColor = 0
        private var sDefaultBackgroundColor = 0
        private fun updateCardBackgroundColor(view: ImageCardView, selected: Boolean) {
            val color = if (selected) sSelectedBackgroundColor else sDefaultBackgroundColor
            // Both background colors should be set because the view's background is temporarily visible
            // during animations.
            view.setBackgroundColor(color)
            view.findViewById<View>(R.id.info_field).setBackgroundColor(color)
        }
    }
}
