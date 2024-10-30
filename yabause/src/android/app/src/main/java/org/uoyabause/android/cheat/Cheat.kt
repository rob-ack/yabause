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
package org.uoyabause.android.cheat

import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.Update
import org.uoyabause.android.GameInfo

/**
 * Created by shinya on 2017/03/04.
 */

@Dao
interface CheatDao{

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insert(cheat: Cheat)

    @Update
    fun update(cheat: Cheat)

    @Delete
    fun delete(cheat: Cheat)

    @Query("SELECT * FROM Cheat")
    fun getAll(): List<Cheat>


    @Query("SELECT * FROM Cheat WHERE gameid = :gameid")
    fun select(gameid: String): List<Cheat>

    @Query("SELECT * FROM Cheat WHERE gameid = :gameid AND enable = 1")
    fun selectEnable(gameid: String): List<Cheat>

    @Query("SELECT * FROM Cheat WHERE gameid = :gameid AND key = :key")
    fun selectKey(gameid: String, key: String): Cheat

    @Query("SELECT * FROM Cheat WHERE gameid = :gameid AND cheat_code = :cheat_code")
    fun selectCheatCode(gameid: String, cheat_code: String): Cheat

    @Query("SELECT * FROM Cheat WHERE gameid = :gameid AND local = 1")
    fun selectLocal(gameid: String): List<Cheat>

    @Query("SELECT * FROM Cheat WHERE gameid = :gameid AND local = 0")
    fun selectRemote(gameid: String): List<Cheat>

    @Query("DELETE FROM Cheat")
    fun deleteAll()

}

@Entity
data class Cheat(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    @ColumnInfo(name = "gameid") var gameid: String? = null,
    @ColumnInfo(name = "description") var description: String? = null,
    @ColumnInfo(name = "cheat_code") var cheat_code: String? = null,
    var key: String? = null,
    var local: Boolean = true,
    var enable: Boolean = false
) {
     constructor(gameid: String?, description: String?, cheat_code: String?):this() {
        this.gameid = gameid
        this.description = description
        this.cheat_code = cheat_code
        this.enable = false
    }
}
