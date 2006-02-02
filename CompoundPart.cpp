/*
 *  CompoundPart.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue May 25 2004.
 *  Copyright (c) 2004 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "CompoundPart.h"
#include "World.h"
#include "c16Image.h"
#include "SDLBackend.h"
#include "Agent.h"

bool partzorder::operator ()(const CompoundPart *s1, const CompoundPart *s2) const {
	// TODO: unsure about all of this, needs a check (but seems to work)
	if (s1->getParent()->getZOrder() == s2->getParent()->getZOrder()) {
		// part 0 is often at the same plane as other parts..
		// TODO: is this correct fix?
		if (s1->getParent() == s2->getParent()) {
			// TODO: evil hack by fuzzie because she's not sure how on earth this ordering works, still :(
			if (dynamic_cast<const FixedTextPart *>(s2)) return true;
			
			return s1->id > s2->id;
		} else
			return s1->getZOrder() > s2->getZOrder();
	}
	return s1->getParent()->getZOrder() > s2->getParent()->getZOrder();
}

creaturesImage *TextEntryPart::caretsprite = 0;

void CompoundPart::render(SDLBackend *renderer, int xoffset, int yoffset) {
	if (parent->visible) {
		partRender(renderer, xoffset + parent->x, yoffset + parent->y);
		if (parent->displaycore /*&& (id == 0)*/) {
			// TODO: tsk, this should be drawn along with the other craziness on the line plane, i expect
			int xoff = xoffset + parent->x + x;
			int yoff = yoffset + parent->y + y;
			renderer->renderLine(xoff + (getWidth() / 2), yoff, xoff + getWidth(), yoff + (getHeight() / 2), 0xFF0000CC);
			renderer->renderLine(xoff + getWidth(), yoff + (getHeight() / 2), xoff + (getWidth() / 2), yoff + getHeight(), 0xFF0000CC);
			renderer->renderLine(xoff + (getWidth() / 2), yoff + getHeight(), xoff, yoff + (getHeight() / 2), 0xFF0000CC);
			renderer->renderLine(xoff, yoff + (getHeight() / 2), xoff + (getWidth() / 2), yoff, 0xFF0000CC);
		}	
	}
}

void SpritePart::partRender(SDLBackend *renderer, int xoffset, int yoffset) {
	assert(getCurrentSprite() < getSprite()->numframes());
	renderer->render(getSprite(), getCurrentSprite(), xoffset + x, yoffset + y, has_alpha, alpha);
}

void SpritePart::setFrameNo(unsigned int f) {
	assert(f < animation.size());
	assert(firstimg + base + animation[f] < getSprite()->numframes());
	
	frameno = f;
	pose = animation[f];
}

void SpritePart::setPose(unsigned int p) {
	assert(firstimg + base + p < getSprite()->numframes());

	animation.clear();
	pose = p;
}
		
void SpritePart::setBase(unsigned int b) {
	assert(firstimg + b + pose < getSprite()->numframes());

	base = b;
}

bool SpritePart::transparentAt(unsigned int x, unsigned int y) {
	return ((duppableImage *)getSprite())->transparentAt(getCurrentSprite(), x, y);
}

void SpritePart::handleClick(float clickx, float clicky) {
	parent->handleClick(clickx + parent->x, clicky + parent->y);
}

CompoundPart::CompoundPart(Agent *p, unsigned int _id, int _x, int _y, int _z) : zorder(_z), parent(p), id(_id) {
	addZOrder();	
	x = _x;
	y = _y;
	
	has_alpha = false;
}

CompoundPart::~CompoundPart() {
	world.zorder.erase(zorder_iter);
}

SpritePart::SpritePart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg,
						int _x, int _y, unsigned int _z) : CompoundPart(p, _id, _x, _y, _z) {
	origsprite = sprite = gallery.getImage(spritefile);
	firstimg = fimg;
	caos_assert(sprite);
	caos_assert(sprite->numframes() > firstimg);
	
	pose = 0;
	base = 0;
	is_transparent = true;
	framerate = 1;
	framedelay = 0;
}

SpritePart::~SpritePart() {
	gallery.delImage(origsprite);
	if (origsprite != sprite) delete sprite;
}

void SpritePart::changeSprite(std::string spritefile, unsigned int fimg) {
	creaturesImage *spr = gallery.getImage(spritefile);
	caos_assert(spr);
	caos_assert(spr->numframes() > fimg);
	// TODO: either assertions to ensure the pose/base are valid, or reset them
	firstimg = fimg;
	gallery.delImage(origsprite);
	// TODO: should we preserve tint?
	if (origsprite != sprite) delete sprite;
	origsprite = sprite = spr;
}

unsigned int CompoundPart::getZOrder() const {
	return parent->getZOrder() + zorder;
}

void CompoundPart::zapZOrder() {
	renderable::zapZOrder();
	world.zorder.erase(zorder_iter);
}

void CompoundPart::addZOrder() {
	renderable::addZOrder();
	zorder_iter = world.zorder.insert(this);	
}

void SpritePart::tint(unsigned char r, unsigned char g, unsigned char b, unsigned char rotation, unsigned char swap) {
	if (origsprite != sprite) delete sprite;
	s16Image *newsprite = new s16Image();
	sprite = newsprite;
	((duppableImage *)origsprite)->duplicateTo(newsprite);
	newsprite->tint(r, g, b, rotation, swap);
}

DullPart::DullPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
			 unsigned int _z) : SpritePart(p, _id, spritefile, fimg, _x, _y, _z) {
}

ButtonPart::ButtonPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
	unsigned int _z, const bytestring &animhover, int msgid, int option) : SpritePart(p, _id, spritefile, fimg, _x, _y, _z) {
	messageid = msgid;
	hitopaquepixelsonly = (option == 1);
	hoveranimation = animhover;
}

unsigned int calculateScriptId(unsigned int message_id); // from caosVM_agent.cpp, TODO: move into shared file

void ButtonPart::handleClick(float x, float y) {
	parent->queueScript(calculateScriptId(messageid), (Agent *)world.hand()); // TODO: pass x/y as p1/p2?
}

TextPart::TextPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y, unsigned int _z, std::string fontsprite)
	                : SpritePart(p, _id, spritefile, fimg, _x, _y, _z) {
	textsprite = gallery.getImage(fontsprite);
	caos_assert(textsprite);
	caos_assert(textsprite->numframes() == 224);
	leftmargin = 8; topmargin = 8; rightmargin = 8; bottommargin = 8;
	linespacing = 0; charspacing = 0;
	horz_align = left; vert_align = top;
	currpage = 0;
	recalculateData(); // ie, insert a blank first page
}

TextPart::~TextPart() {
	for (std::vector<texttintinfo>::iterator i = tints.begin(); i != tints.end(); i++)
		if (i->sprite != textsprite)
			delete i->sprite;	
	gallery.delImage(textsprite);
}

void TextPart::addTint(std::string tintinfo) {
	// add a tint, starting at text.size(), using the data in tintinfo
	// TODO: there's some caching to be done here, but tinting is rather rare, so..
	
	unsigned short r = 128, g = 128, b = 128, rot = 128, swap = 128;
	int where = 0;
	std::string cur;
	for (unsigned int i = 0; i <= tintinfo.size(); i++) {
		if (i == tintinfo.size() || tintinfo[i] == ' ') {
			unsigned short val = atoi(cur.c_str());
			if (val <= 256) {
				switch (where) {
					case 0: r = val; break;
					case 1: g = val; break;
					case 2: b = val; break;
					case 3: rot = val; break;
					case 4: swap = val; break;
				}
			} // TODO: else explode();
			where++;
			cur = "";
			if (where > 4) break;
		} else cur += tintinfo[i];
	}

	texttintinfo t;
	t.offset = text.size();

	if (!(r == g == b == rot == swap == 128)) {
		s16Image *tintedsprite = new s16Image();
		((duppableImage *)textsprite)->duplicateTo(tintedsprite);
		tintedsprite->tint(r, g, b, rot, swap);
		t.sprite = tintedsprite;
	} else t.sprite = textsprite;

	tints.push_back(t);
}

void TextPart::setText(std::string t) {
	text.clear();
	for (std::vector<texttintinfo>::iterator i = tints.begin(); i != tints.end(); i++)
		if (i->sprite != textsprite)
			delete i->sprite;
	tints.clear();

	// parse and remove the <tint> tagging
	for (unsigned int i = 0; i < t.size(); i++) {
		if ((t[i] == '<') && (t.size() > i+4))
			if ((t[i + 1] == 't') && (t[i + 2] == 'i') && (t[i + 3] == 'n') && (t[i + 4] == 't')) {
				i += 5;
				std::string tintinfo;
				if (t[i] == ' ') i++; // skip initial space, if any
				for (; i < t.size(); i++) {
					if (t[i] == '>') 
						break;
					tintinfo += t[i];
				}
				addTint(tintinfo);
				continue;
			}
		text += t[i];
	}
	
	recalculateData();
}

void TextEntryPart::setText(std::string t) {
	TextPart::setText(t);

	// place caret at the end of the text
	caretpos = text.size();
}

unsigned int calculateScriptId(unsigned int message_id); // from caosVM_agent.cpp, TODO: move into shared file

void TextEntryPart::handleClick(float clickx, float clicky) {
	world.setFocus(this);
}

void TextEntryPart::handleKey(char c) {
	text += c;
	caretpos++;
	recalculateData();
}

void TextEntryPart::handleSpecialKey(char c) {
	switch (c) {
		case 8: // backspace
			if (text.size() == 0) return;
			text.erase(caretpos - 1, caretpos); // TODO: broken
			caretpos--;
			break;

		case 13: // return
			// TODO: check if we should do this or a newline
			parent->queueScript(calculateScriptId(messageid), 0); // TODO: is a null FROM correct?
			break;

		case 37: // left
			if (caretpos == 0) return;
			caretpos--;
			break;

		case 39: // right
			if (caretpos == text.size()) return;
			caretpos++;
			break;

		case 38: // up
		case 40: // down
			break;

		case 46: // delete
			if ((text.size() == 0) || (caretpos >= text.size()))
				return;
			text.erase(caretpos);
			break;

		default:
			return;
	}
	recalculateData();
}

void TextPart::setFormat(int left, int top, int right, int bottom, int line, int _char, horizontalalign horza, verticalalign verta, bool lastpagescroll) {
	leftmargin = left;
	topmargin = top;
	rightmargin = right;
	bottommargin = bottom;
	linespacing = line;
	charspacing = _char;
	horz_align = horza;
	vert_align = verta;
	last_page_scroll = lastpagescroll;
	recalculateData();
}

unsigned int TextPart::calculateWordWidth(std::string word) {
	unsigned int x = 0;
	for (unsigned int i = 0; i < word.size(); i++) {
		if (word[i] < 32) continue; // TODO: replace with space or similar?
		int spriteid = word[i] - 32;	

		x += textsprite->width(spriteid);
		if (i != 0) x += charspacing;
	}
	return x;
}

/*
 * Recalculate the data used for rendering the text part.
 */
void TextPart::recalculateData() {
	linedata currentdata;

	lines.clear();
	pages.clear();
	pageheights.clear();
	pages.push_back(0);
	if (text.size() == 0) {
		pageheights.push_back(0);
		lines.push_back(currentdata); // blank line, so caret is rendered in TextEntryParts
		return;
	}

	unsigned int textwidth = getWidth() - leftmargin - rightmargin;
	unsigned int textheight = getHeight() - topmargin - bottommargin;

	unsigned int currenty = 0, usedheight = 0;
	unsigned int i = 0;
	while (i < text.size()) {
		bool newline = false;
		unsigned int startoffset = i;
		// first, retrieve a word from the text
		std::string word;
		for (; i < text.size(); i++) {
			if ((text[i] == ' ') || (text[i] == '\n')) {
				if (text[i] == '\n') newline = true;
				i++;
				break;
			}
			word += text[i];
		}

		// next, work out whether it fits
		unsigned int wordlen = calculateWordWidth(word);
		unsigned int spacelen = textsprite->width(0) + charspacing;
		unsigned int possiblelen = wordlen;
		if (currentdata.text.size() > 0)
			possiblelen = wordlen + spacelen;
		// TODO: set usedheight as appropriate/needed
		usedheight = textsprite->height(0);
		if (currentdata.width + possiblelen <= textwidth) {
			// the rest of the word fits on the current line, so that's okay.
			// add a space if we're not the first word on this line
			if (currentdata.text.size() > 0) word = std::string(" ") + word;
			currentdata.text += word;
			currentdata.width += possiblelen;
		} else if (wordlen <= textwidth) {
			// the rest of the word doesn't fit on the current line, but does on the next line.
			if (currenty + usedheight > textheight) {
				pageheights.push_back(currenty);
				pages.push_back(lines.size());
				currenty = 0;
			} else currenty += usedheight + linespacing;
			currentdata.text += " "; // TODO: HACK THINK ABOUT THIS
			lines.push_back(currentdata);
			currentdata.reset(startoffset);

			currentdata.text += word;
			currentdata.width += wordlen;
		} else {
			// TODO: word is too wide to fit on a single line
			// we should output as much as possible and then go backwards
		}

		// we force a newline here if necessary (ie, if the last char is '\n', except not in the last case)
		if ((i < text.size()) && (newline)) {
			if (currenty + usedheight > textheight) {
				pageheights.push_back(currenty);
				pages.push_back(lines.size());
				currenty = 0;
			} else currenty += usedheight + linespacing;
			lines.push_back(currentdata);
			currentdata.reset(startoffset);
		}
	}

	if (currentdata.text.size() > 0) {
		currenty += usedheight;
		if (text[text.size() - 1] == ' ') currentdata.text += " "; // TODO: HACK THINK ABOUT THIS
		lines.push_back(currentdata);
	}
	
	pageheights.push_back(currenty);
}

void TextPart::partRender(SDLBackend *renderer, int xoffset, int yoffset, TextEntryPart *caretdata) {
	SpritePart::partRender(renderer, xoffset + x, yoffset + y);
	
	unsigned int xoff = xoffset + x + leftmargin;
	unsigned int yoff = yoffset + y + topmargin;
	unsigned int textwidth = getWidth() - leftmargin - rightmargin;
	unsigned int textheight = getHeight() - topmargin - bottommargin;

	unsigned int currenty = 0, usedheight = 0;
	if (vert_align == bottom)
		currenty = textheight - pageheights[currpage];
	else if (vert_align == middle)
		currenty = (textheight - pageheights[currpage]) / 2;
	unsigned int startline = pages[currpage];
	unsigned int endline = (currpage + 1 < pages.size() ? pages[currpage + 1] : lines.size());
	creaturesImage *sprite = textsprite; unsigned int currtint = 0;
	for (unsigned int i = startline; i < endline; i++) {	
		unsigned int currentx = 0, somex = xoff;
		if (horz_align == right)
			somex = somex + (textwidth - lines[i].width);
		else if (horz_align == center)
			somex = somex + ((textwidth - lines[i].width) / 2);

		for (unsigned int x = 0; x < lines[i].text.size(); x++) {
			if (currtint < tints.size() && tints[currtint].offset == lines[i].offset + x) {
				sprite = tints[currtint].sprite;
				currtint++;
			}
		
			if (lines[i].text[x] < 32) continue; // TODO: replace with space or similar?
			int spriteid = lines[i].text[x] - 32;
			renderer->render(sprite, spriteid, somex + currentx, yoff + currenty, has_alpha, alpha);
			if ((caretdata) && (caretdata->caretpos == lines[i].offset + x))
				caretdata->renderCaret(renderer, somex + currentx, yoff + currenty);
			currentx += textsprite->width(spriteid) + charspacing;
		}
		if ((caretdata) && (caretdata->caretpos == lines[i].offset + lines[i].text.size()))
			caretdata->renderCaret(renderer, somex + currentx, yoff + currenty);		
		currenty += textsprite->height(0) + linespacing;
	}
}

FixedTextPart::FixedTextPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
		                                  unsigned int _z, std::string fontsprite) : TextPart(p, _id, spritefile, fimg, _x, _y, _z, fontsprite) {
	// nothing, hopefully.. :)
}

TextEntryPart::TextEntryPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
		                                  unsigned int _z, unsigned int msgid, std::string fontsprite) : TextPart(p, _id, spritefile, fimg, _x, _y, _z, fontsprite) {
	// TODO: hm, this never gets freed..
	if (!caretsprite) { caretsprite = gallery.getImage("cursor"); caos_assert(caretsprite); }

	caretpose = 0;
	caretpos = 0;
	focused = false;
	messageid = msgid;
}

void TextEntryPart::partRender(SDLBackend *renderer, int xoffset, int yoffset) {
	TextPart::partRender(renderer, xoffset, yoffset, (focused ? this : 0));
}

void TextEntryPart::renderCaret(SDLBackend *renderer, int xoffset, int yoffset) {
	// TODO: fudge xoffset/yoffset as required
	renderer->render(caretsprite, caretpose, xoffset, yoffset, has_alpha, alpha);
}

void TextEntryPart::tick() {
	CompoundPart::tick();

	if (focused) {
		caretpose++;
		if (caretpose == caretsprite->numframes())
			caretpose = 0;
	}
}

void SpritePart::tick() {
	if (!animation.empty()) {
                if (framerate > 1) {
			framedelay++;
			if (framedelay == framerate + 1)
				framedelay = 0;
		}
		
		if (framedelay == 0) {
			unsigned int f = frameno + 1;
			if (f == animation.size()) return;
			if (animation[f] == 255) {
				if (f == (animation.size() - 1)) f = 0;
				else f = animation[f + 1];
			}
			// TODO: check f is valid..
			setFrameNo(f);
		}
	}
}

CameraPart::CameraPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
		unsigned int _z, unsigned int viewwidth, unsigned int viewheight, unsigned int camerawidth, unsigned int cameraheight)
		: SpritePart(p, _id, spritefile, fimg, _x, _y, _z) {
	// TODO: set viewwidth/viewheight and use for getWidth/getHeight, store camerawidth/cameraheight
}

GraphPart::GraphPart(Agent *p, unsigned int _id, std::string spritefile, unsigned int fimg, int _x, int _y,
		unsigned int _z, unsigned int novalues) : SpritePart(p, _id, spritefile, fimg, _x, _y, _z) {
	// TODO: store novalues
}

/* vim: set noet: */

