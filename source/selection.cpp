//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////
// $URL: http://svn.rebarp.se/svn/RME/trunk/source/selection.hpp $
// $Id: selection.hpp 310 2010-02-26 18:03:48Z admin $

#include "main.h"

#include "selection.h"
#include "tile.h"
#include "creature.h"
#include "item.h"
#include "editor.h"
#include "gui.h"

SelectionArea::SelectionArea(Editor& editor) :
	busy(false),
	editor(editor),
	session(nullptr),
	subsession(nullptr)
{
	////
}

SelectionArea::~SelectionArea()
{
	delete subsession;
	delete session;
}

Position SelectionArea::minPosition() const
{
	Position minPos(0x10000, 0x10000, 0x10);
	for(TileVector::const_iterator tile = tiles.begin(); tile != tiles.end(); ++tile) {
		Position pos((*tile)->getPosition());
		if(minPos.x > pos.x)
			minPos.x = pos.x;
		if(minPos.y > pos.y)
			minPos.y = pos.y;
		if(minPos.z > pos.z)
			minPos.z = pos.z;
	}
	return minPos;
}

Position SelectionArea::maxPosition() const
{
	Position maxPos(0, 0, 0);
	for(TileVector::const_iterator tile = tiles.begin(); tile != tiles.end(); ++tile) {
		Position pos((*tile)->getPosition());
		if(maxPos.x < pos.x)
			maxPos.x = pos.x;
		if(maxPos.y < pos.y)
			maxPos.y = pos.y;
		if(maxPos.z < pos.z)
			maxPos.z = pos.z;
	}
	return maxPos;
}

void SelectionArea::add(Tile* tile, Item* item)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(item);

	if(item->isSelected()) return;

	// Make a copy of the tile with the item selected
	item->select();
	Tile* new_tile = tile->deepCopy(editor.map);
	item->deselect();

	if(g_settings.getInteger(Config::BORDER_IS_GROUND))
		if(item->isBorder())
			new_tile->selectGround();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::add(Tile* tile, Spawn* spawn)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(spawn);

	if(spawn->isSelected()) return;

	// Make a copy of the tile with the item selected
	spawn->select();
	Tile* new_tile = tile->deepCopy(editor.map);
	spawn->deselect();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::add(Tile* tile, Creature* creature)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(creature);

	if(creature->isSelected()) return;

	// Make a copy of the tile with the item selected
	creature->select();
	Tile* new_tile = tile->deepCopy(editor.map);
	creature->deselect();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::add(Tile* tile)
{
	ASSERT(subsession);
	ASSERT(tile);

	Tile* new_tile = tile->deepCopy(editor.map);
	new_tile->select();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::remove(Tile* tile, Item* item)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(item);

	bool tmp = item->isSelected();
	item->deselect();
	Tile* new_tile = tile->deepCopy(editor.map);
	if(tmp) item->select();
	if(item->isBorder() && g_settings.getInteger(Config::BORDER_IS_GROUND)) new_tile->deselectGround();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::remove(Tile* tile, Spawn* spawn)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(spawn);

	bool tmp = spawn->isSelected();
	spawn->deselect();
	Tile* new_tile = tile->deepCopy(editor.map);
	if(tmp) spawn->select();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::remove(Tile* tile, Creature* creature)
{
	ASSERT(subsession);
	ASSERT(tile);
	ASSERT(creature);

	bool tmp = creature->isSelected();
	creature->deselect();
	Tile* new_tile = tile->deepCopy(editor.map);
	if(tmp) creature->select();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::remove(Tile* tile)
{
	ASSERT(subsession);

	Tile* new_tile = tile->deepCopy(editor.map);
	new_tile->deselect();

	subsession->addChange(newd Change(new_tile));
}

void SelectionArea::addInternal(Tile* tile)
{
	ASSERT(tile);

	tiles.push_back(tile);
	erase_iterator = tiles.begin();
}

void SelectionArea::removeInternal(Tile* tile)
{
	ASSERT(tile);
#ifdef __DEBUG_MODE__
	TileVector::iterator erase_begin = erase_iterator;
#endif
	if(tiles.size() == 0)
		return;

	TileVector::iterator end_iter = tiles.end();
	do {
		if(erase_iterator == end_iter) {
			erase_iterator = tiles.begin();
		}
		if(*erase_iterator == tile) {
			// Swap & pop trick
			if(*erase_iterator == tiles.back()) {
				tiles.pop_back();
				erase_iterator = tiles.begin();
			} else {
				std::swap(*erase_iterator, tiles.back());
				tiles.pop_back();
				erase_iterator = tiles.begin() + (erase_iterator - tiles.begin());
			}
			return;
		}
		++erase_iterator;
#ifdef __DEBUG_MODE__
		ASSERT(/* This shouldn't happend*/ erase_iterator != erase_begin);
#endif
	} while(true);
}

void SelectionArea::clear()
{
	if(session) {
		for(TileVector::iterator it = tiles.begin(); it != tiles.end(); it++) {
			Tile* new_tile = (*it)->deepCopy(editor.map);
			new_tile->deselect();
			subsession->addChange(newd Change(new_tile));
		}
	} else {
		for(TileVector::iterator it = tiles.begin(); it != tiles.end(); it++) {
			(*it)->deselect();
		}
		tiles.clear();
	}
}

void SelectionArea::start(SessionFlags flags)
{
	if(!(flags & INTERNAL)) {
		if(flags & SUBTHREAD) {
			;
		} else {
			session = editor.actionQueue->createBatch(ACTION_SELECT);
		}
		subsession = editor.actionQueue->createAction(ACTION_SELECT);
	}
	erase_iterator = tiles.begin();
	busy = true;
}

void SelectionArea::commit()
{
	erase_iterator = tiles.begin();
	if(session) {
		ASSERT(subsession);
		// We need to step out of the session before we do the action, else peril awaits us!
		BatchAction* tmp = session;
		session = nullptr;

		// Do the action
		tmp->addAndCommitAction(subsession);

		// Create a newd action for subsequent selects
		subsession = editor.actionQueue->createAction(ACTION_SELECT);
		session = tmp;
	}
}

void SelectionArea::finish(SessionFlags flags)
{
	erase_iterator = tiles.begin();
	if(!(flags & INTERNAL)) {
		if(flags & SUBTHREAD) {
			ASSERT(subsession);
			subsession = nullptr;
		} else {
			ASSERT(session);
			ASSERT(subsession);
			// We need to exit the session before we do the action, else peril awaits us!
			BatchAction* tmp = session;
			session = nullptr;

			tmp->addAndCommitAction(subsession);
			editor.addBatch(tmp, 2);

			session = nullptr;
			subsession = nullptr;
		}
	}
	busy = false;
}

void SelectionArea::updateSelectionCount()
{
	if(size() > 0) {
		wxString ss;
		if(size() == 1) {
			ss << wxT("One tile selected.");
		} else {
			ss << size() << wxT(" tiles selected.");
		}
		g_gui.SetStatusText(ss);
	}
}

void SelectionArea::join(SelectionThread* thread)
{
	thread->Wait();

	ASSERT(session);
	session->addAction(thread->result);
	thread->selection.subsession = nullptr;

	delete thread;
}

SelectionThread::SelectionThread(Editor& editor, Position start, Position end) :
	wxThread(wxTHREAD_JOINABLE),
	editor(editor),
	start(start),
	end(end),
	positions(nullptr),
	selection(editor),
	result(nullptr)
{
	////
}

SelectionThread::SelectionThread(Editor& editor, PositionVector* positions) :
	wxThread(wxTHREAD_JOINABLE),
	editor(editor),
	start(),
	end(),
	positions(positions),
	selection(editor),
	result(nullptr)
{
	////
}

SelectionThread::~SelectionThread()
{
	////
}

void SelectionThread::Execute()
{
	Create();
	Run();
}

wxThread::ExitCode SelectionThread::Entry()
{
	selection.start(SelectionArea::SUBTHREAD);
	Map& map = editor.map;

	if(positions) {
		for(int i = 0; i < positions->size(); ++i) {
			const Position& position = (*positions)[i];
			Tile* tile = map.getTile(position);
			if(!tile)
				continue;

			selection.add(tile);
		}
	} else {
		for(int z = start.z; z >= end.z; --z) {
			for(int x = start.x; x <= end.x; ++x) {
				for(int y = start.y; y <= end.y; ++y) {
					Tile* tile = map.getTile(x, y, z);
					if(!tile)
						continue;

					selection.add(tile);
				}
			}
			if(z <= GROUND_LAYER && g_settings.getInteger(Config::COMPENSATED_SELECT)) {
				++start.x; ++start.y;
				++end.x; ++end.y;
			}
		}
	}
	
	result = selection.subsession;
	selection.finish(SelectionArea::SUBTHREAD);
	return nullptr;
}

