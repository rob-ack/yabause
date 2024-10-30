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

import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Database
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import androidx.room.Update
import java.util.Date

/**
 * Created by shinya on 2016/01/04.
 */

@Dao
interface GameStatusDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insert(gameStatus: GameStatus)

    @Update
    fun update(gameStatus: GameStatus)

    @androidx.room.Delete
    fun delete(gameStatus: GameStatus)

    @Query("SELECT * FROM GameStatus ORDER BY update_at DESC LIMIT 1")
    fun getLatestGameStatus(): GameStatus?

    @Query("SELECT * FROM GameStatus WHERE product_number = :product_number")
    fun select(product_number: String): GameStatus?

/*
    fun selectAll(): List<GameStatus>
    fun select(product_number: String): GameStatus?
    fun selectByRating(rating: Int): List<GameStatus>
    fun selectByUpdateAt(update_at: Date): List<GameStatus>
    fun selectByUpdateAt(update_at: Date, rating: Int): List<GameStatus>
    fun selectByUpdateAt(update_at: Date, rating: Int, product_number: String): List<GameStatus>
    fun selectByUpdateAt(update_at: Date, product_number: String): List<GameStatus>
    fun selectByRating(rating: Int, product_number: String): List<GameStatus>

 */
}

@Entity
data class GameStatus(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    @ColumnInfo(name = "product_number", index = true) var product_number: String = "",
    @ColumnInfo(name = "update_at") var update_at: Date? = null,
    @ColumnInfo(name = "image_url") var image_url:String = "",
    @ColumnInfo(name = "rating") var rating: Int = -1
) {
    constructor(product_number: String, update_at: Date, image_url: String, rating: Int) : this() {
        this.product_number = product_number
        this.update_at = update_at
        this.image_url = image_url
        this.rating = rating
    }

    companion object {
        val lastUpdate: Date?
            get() {
                val tmp = YabauseStorage.gameStatusDao.getLatestGameStatus()
                return tmp?.update_at
            }
    }
}
