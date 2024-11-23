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

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.GameInfo
import org.uoyabause.android.YabauseStorage
import org.uoyabause.android.YabauseStorage.Companion.storage

/**
 * Created by shinya on 2016/01/03.
 */
class RecyclerGameListAdapter : RecyclerView.Adapter<RecyclerGameListAdapter.ViewHolder>() {
    var _gamelist: List<GameInfo>

    // Provide a reference to the views for each data item
    // Complex data items may need more than one view per item, and
    // you provide access to all the views for a data item in a view holder
    class ViewHolder(
// each data item is just a string in this case
        var _cardview: CardView
    ) : RecyclerView.ViewHolder(_cardview) {
        var _title: TextView = _cardview.findViewById<View>(R.id.game_title) as TextView
    }

    // Provide a suitable constructor (depends on the kind of dataset)
    init {
        val yb = storage
        yb.generateGameDB(3)
        _gamelist = YabauseStorage.dao.getAllSortedByTitle()
    }

    // Create new views (invoked by the layout manager)
    override fun onCreateViewHolder(
        parent: ViewGroup,
        viewType: Int
    ): ViewHolder {
        // create a new view
        val v = LayoutInflater.from(parent.context)
            .inflate(R.layout.game_card_listitem, parent, false) as CardView
        // set the view's size, margins, paddings and layout parameters
        val vh = ViewHolder(v)
        return vh
    }

    // Replace the contents of a view (invoked by the layout manager)
    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element
        //holder.mTextView.setText(mDataset[position]);

        holder._title.text = _gamelist[position].game_title
    }

    // Return the size of your dataset (invoked by the layout manager)
    override fun getItemCount(): Int {
        return _gamelist.size
    }
}
