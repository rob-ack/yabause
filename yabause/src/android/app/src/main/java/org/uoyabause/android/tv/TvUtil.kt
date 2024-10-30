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

import android.content.ContentUris
import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.drawable.VectorDrawable
import android.media.tv.TvContract
import android.net.Uri
import android.util.Log
import androidx.annotation.WorkerThread
import androidx.tvprovider.media.tv.Channel
import androidx.tvprovider.media.tv.ChannelLogoUtils
import androidx.tvprovider.media.tv.PreviewProgram
import androidx.tvprovider.media.tv.TvContractCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import org.uoyabause.android.GameInfo
import org.uoyabause.android.YabauseStorage
import java.net.URLEncoder

object TvUtil {
    private const val TAG = "TvUtil"
    private const val CHANNEL_JOB_ID_OFFSET: Long = 1000

    private val CHANNELS_PROJECTION = arrayOf(
        TvContractCompat.Channels._ID,
        TvContract.Channels.COLUMN_DISPLAY_NAME,
        TvContractCompat.Channels.COLUMN_BROWSABLE
    )

    /**
     * Converts a [Subscription] into a [Channel] and adds it to the tv provider.
     *
     * @param context used for accessing a content resolver.
     * @param subscription to be converted to a channel and added to the tv provider.
     * @return the id of the channel that the tv provider returns.
     */
    @WorkerThread
    fun createChannel(context: Context, subscription: Subscription): Long {
        // Checks if our subscription has been added to the channels before.

        val cursor =
            context.contentResolver
                .query(
                    TvContractCompat.Channels.CONTENT_URI,
                    CHANNELS_PROJECTION,
                    null,
                    null,
                    null
                )
        if (cursor != null && cursor.moveToFirst()) {
            do {
                val channel = Channel.fromCursor(cursor)
                if (subscription.name == channel.displayName) {
                    Log.d(
                        TAG,
                        "Channel already exists. Returning channel "
                                + channel.id
                                + " from TV Provider."
                    )
                    return channel.id
                }
            } while (cursor.moveToNext())
        }

        // Create the channel since it has not been added to the TV Provider.
        val appLinkIntentUri = Uri.parse(subscription.appLinkIntentUri)

        val builder = Channel.Builder()
        builder.setType(TvContractCompat.Channels.TYPE_PREVIEW)
            .setDisplayName(subscription.name)
            .setDescription(subscription.description)
            .setAppLinkIntentUri(appLinkIntentUri)

        Log.d(TAG, "Creating channel: " + subscription.name)
        val channelUrl =
            context.contentResolver
                .insert(
                    TvContractCompat.Channels.CONTENT_URI,
                    builder.build().toContentValues()
                )

        Log.d(TAG, "channel insert at $channelUrl")
        val channelId = ContentUris.parseId(channelUrl!!)
        Log.d(TAG, "channel id $channelId")

        val bitmap = convertToBitmap(context, subscription.channelLogo)
        if (bitmap != null) {
            ChannelLogoUtils.storeChannelLogo(context, channelId, bitmap)
        }
        TvContractCompat.requestChannelBrowsable(context, channelId)
        return channelId
    }

    fun convertToBitmap(context: Context, resourceId: Int): Bitmap {
        val drawable = context.getDrawable(resourceId)
        if (drawable is VectorDrawable) {
            val bitmap =
                Bitmap.createBitmap(
                    drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight(),
                    Bitmap.Config.ARGB_8888
                )
            val canvas = Canvas(bitmap)
            drawable.setBounds(0, 0, canvas.width, canvas.height)
            drawable.draw(canvas)
            return bitmap
        }

        return BitmapFactory.decodeResource(context.resources, resourceId)
    }

    fun syncPrograms(context: Context, channelId: Long) {
        Log.d(TAG, "Sync programs for channel: $channelId")

        context.contentResolver
            .query(
                TvContractCompat.buildChannelUri(channelId),
                null,
                null,
                null,
                null
            ).use { cursor ->
                if (cursor != null && cursor.moveToNext()) {
                    val channel = Channel.fromCursor(cursor)
                    if (!channel.isBrowsable) {
                        Log.d(TAG, "Channel is not browsable: $channelId")
                        //deletePrograms(channelId, movies);
                        deletePrograms(context, channelId)
                    } else {
                        Log.d(TAG, "Channel is browsable: $channelId")
                        deletePrograms(context, channelId)
                        createPrograms(context, channelId)
                        //if (movies.isEmpty()) {
                        //   movies = createPrograms(channelId, MockMovieService.getList());
                        //} else {
                        //    movies = updatePrograms(channelId, movies);
                        //}
                        //MockDatabase.saveMovies(context, channelId, movies);
                    }
                }
            }
    }

    fun createPrograms(context: Context, channelId: Long)/*: List<GameInfo>?*/ {

        GlobalScope.launch(Dispatchers.IO) {
            var recentGameList: List<GameInfo>? = null
            try {

                recentGameList = YabauseStorage.dao.getRecentGames()

            } catch (e: Exception) {
                println(e)
            }

            if( recentGameList != null ) {
                for (game in recentGameList) {
                    val previewProgram = buildProgram(channelId, game)

                    val programUri =
                        context.contentResolver
                            .insert(
                                TvContractCompat.PreviewPrograms.CONTENT_URI,
                                previewProgram?.toContentValues()
                            )
                    val programId = ContentUris.parseId(programUri!!)
                    Log.d(TAG, "Inserted new program: $programId")
                    //game.setProgramId(programId); ToDo
                    //moviesAdded.add(movie);
                }
            }
        }
        return /*recentGameList*/
    }

    fun deletePrograms(context: Context, channelId: Long) {
        context.contentResolver
            .delete(
                TvContractCompat.PreviewPrograms.CONTENT_URI,
                null,
                null
            )
    }

    private const val SCHEMA_URI_PREFIX = "saturngame://yabasanshiro/"
    const val PLAY: String = "play"
    private const val URI_PLAY = SCHEMA_URI_PREFIX + PLAY

    fun buildProgram(channelId: Long, game: GameInfo): PreviewProgram? {
        val posterArtUri = Uri.parse(game.image_url)

        //Uri appLinkUri = AppLinkHelper.buildPlaybackUri(channelId, movie.getId());
        //Uri previewVideoUri = Uri.parse(movie.getVideoUrl());
        val encodedResult: String

        try {
            encodedResult = URLEncoder.encode(game.file_path, "UTF-8")
        } catch (e: Exception) {
            return null
        }

        val appLinkUri = Uri.parse(URI_PLAY).buildUpon().appendPath(encodedResult).build()

        val builder = PreviewProgram.Builder()
        builder.setChannelId(channelId)
            .setType(TvContractCompat.PreviewProgramColumns.TYPE_CLIP)
            .setTitle(game.game_title)
            .setDescription(game.maker_id)
            .setIntentUri(appLinkUri)
            .setPosterArtUri(posterArtUri)

        //.setPreviewVideoUri(previewVideoUri)
        return builder.build()
    }
}
