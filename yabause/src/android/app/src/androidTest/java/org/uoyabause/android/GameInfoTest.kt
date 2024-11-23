package org.uoyabause.android

import androidx.room.Room
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.uoyabause.android.GameInfo
import java.io.BufferedWriter
import java.io.File
import java.io.FileWriter

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
@RunWith(AndroidJUnit4::class)
class GameInfoTest {
    @Rule
    var tempFolder: TemporaryFolder = TemporaryFolder()

    val db: GameInfoDatabase by lazy {
        Room.databaseBuilder(
            YabauseApplication.appContext,
            GameInfoDatabase::class.java, "test-database"
        ).build()
    }
    val dao: GameInfoDao by lazy {
        db.gameInfoDao()
    }

    @Before
    fun cleanTable() {
        //new Delete().from(GameInfo.class).execute();
    }

    @Test
    fun removeInstance_ccdFilesAreRemoved() {
        // SetUpTest

        try {
            tempFolder.newFile("DoDonPachi (JP).ccd")
            tempFolder.newFile("DoDonPachi (JP).img")
        } catch (e: Exception) {
            Assert.assertEquals(1, -1)
        }
        val len_before = tempFolder.root.list().size
        Assert.assertEquals(2, len_before.toLong())

        val game = GameInfo()
        game.file_path = tempFolder.root.toString() + "/DoDonPachi (JP).ccd"
        dao.insertAll(game)

        // Execute
        game.removeInstance()

        // Validation
        val list = dao.getAll()
        val count = list.count()
        Assert.assertEquals(0, count.toLong())

        val len = tempFolder.root.list().size
        Assert.assertEquals(0, len.toLong())
    }


    @Test
    fun removeInstance_mdsFilesAreRemoved() {
        // SetUpTest

        try {
            tempFolder.newFile("Doom (U).mds")
            tempFolder.newFile("Doom (U).mdf")
        } catch (e: Exception) {
            Assert.assertEquals(1, -1)
        }
        val len_before = tempFolder.root.list().size
        Assert.assertEquals(2, len_before.toLong())

        val game = GameInfo()
        game.file_path = tempFolder.root.toString() + "/Doom (U).mds"
        dao.insertAll(game)

        // Execute
        game.removeInstance()

        // Validation
        val list = dao.getAll()
        val count = list.count()
        Assert.assertEquals(0, count.toLong())

        val len = tempFolder.root.list().size
        Assert.assertEquals(0, len.toLong())
    }


    @Test
    fun removeInstance_chdFilesAreRemoved() {
        // SetUpTest

        try {
            tempFolder.newFile("Assault Suit Leynos 2 (Japan).cue.chd")
        } catch (e: Exception) {
            Assert.assertEquals(1, -1)
        }
        val len_before = tempFolder.root.list().size
        Assert.assertEquals(1, len_before.toLong())

        val game = GameInfo()
        game.file_path = tempFolder.root.toString() + "/Assault Suit Leynos 2 (Japan).cue.chd"
        dao.insertAll(game)

        // Execute
        game.removeInstance()

        // Validation
        val list = dao.getAll()
        val count = list.count()
        Assert.assertEquals(0, count.toLong())

        val len = tempFolder.root.list().size
        Assert.assertEquals(0, len.toLong())
    }


    @Test
    fun removeInstance_cueFilesAreRemoved() {
        // SetUpTest

        val f = File(tempFolder.root, "Assault Suit Leynos 2 (Japan).cue")
        try {
            val fww = FileWriter(f)
            val fw = BufferedWriter(FileWriter(f))
            fw.write("CATALOG 0000000000000")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 01).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 01 MODE1/2352")
            fw.newLine()
            fw.write("INDEX 01 00:00:00")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 02).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 02 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:02:00")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 03).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 03 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 04).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 04 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 05).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 05 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 06).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 06 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 07).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 07 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 08).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 08 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 09).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 09 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 10).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 10 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 11).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 11 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 12).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 12 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 13).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 13 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.write("FILE \"Assault Suit Leynos 2 (Japan) (Track 14).bin\" BINARY")
            fw.newLine()
            fw.write("TRACK 14 AUDIO")
            fw.newLine()
            fw.write("INDEX 00 00:00:00")
            fw.newLine()
            fw.write("INDEX 01 00:01:74")
            fw.newLine()
            fw.close()

            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 01).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 02).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 03).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 04).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 05).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 06).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 07).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 08).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 09).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 10).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 11).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 12).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 13).bin")
            tempFolder.newFile("Assault Suit Leynos 2 (Japan) (Track 14).bin")

            tempFolder.newFile("dmy.cue")
        } catch (e: Exception) {
            Assert.assertEquals(1, -1)
        }

        val len_before = tempFolder.root.list().size
        Assert.assertEquals(16, len_before.toLong())

        val game = GameInfo()
        game.file_path = tempFolder.root.toString() + "/Assault Suit Leynos 2 (Japan).cue"
        dao.insertAll(game)

        // Execute
        game.removeInstance()

        // Validation
        val list = dao.getAll()
        val count = list.count()
        Assert.assertEquals(0, count.toLong())

        val len = tempFolder.root.list().size
        Assert.assertEquals(1, len.toLong())
    }
}