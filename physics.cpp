/*
 *  physics.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue 08 Feb 2005.
 *  Copyright (c) 2005 Alyssa Milburn. All rights reserved.
 *  Copyright (c) 2005 Bryan Donlan. All rights reserved.
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
#include "physics.h"

void Line::dump() const {
	std::cout << "pst = (" << start.x << "," << start.y << ") end=(" << end.x << "," << end.y << ")" << std::endl;
	std::cout << "xi = " << x_icept << " yi = " << y_icept << " m=" << slope << std::endl;
	std::cout << "type = ";
	switch (type) {
		case NORMAL: std::cout << "NORMAL"; break;
		case HORIZONTAL: std::cout << "HORIZ"; break;
		case VERTICAL: std::cout << "VERT"; break;
		default: std::cout << "?? (" << type << ")"; break;
	}
	std::cout << std::endl;
	sanity_check();
}

Line::Line(Point s, Point e) {
	if (s.x > e.x)
		std::swap(s, e);
	start = s;
	end = e;
		
	if (s.x == e.x) {
		type = VERTICAL;
		x_icept = s.x;
	} else if (s.y == e.y) {
		type = HORIZONTAL;
		y_icept = s.y;
		slope = 0;
	} else {
		type = NORMAL;
		slope = (end.y - start.y) / (end.x - start.x);
		/* y = mx + b
		 * b = y - mx
		 */
		y_icept = start.y - slope * start.x;
		/* 0 = mx + b
		 * x = -b/m
		 */
		x_icept = -y_icept/slope;
	}
}

static inline bool isBetween(float x, float a, float b) {
	return (a <= x && x <= b) || (b <= x && x <= a);
}

bool Line::intersect(const Line &l, Point &where) const {
	if (type == VERTICAL) {
		if (l.type == VERTICAL) return false; // TODO?
		float x1 = l.start.x,
					x2 = l.end.x;
		float y = l.pointAtX(start.x).y;
		if (isBetween(start.x, x1, x2) && isBetween(y, start.y, end.y)) {
			where = Point(start.x, y);
			return true;
		}
		return false;
	}
	if (l.type == VERTICAL) {
		if (type == VERTICAL) return false; // TODO?
		float x1 = start.x,
					x2 = end.x;
		float y = pointAtX(l.start.x).y;
		if (isBetween(l.start.x, x1, x2) && isBetween(y, l.start.y, l.end.y)) {
			where = Point(l.start.x, y);
			return true;
		}
		return false;
	}
	if (slope == l.slope) {
		return false; // TODO: maybe test if the lines really do intersect? no real meaning to it though
	}
	float c = start.y - slope * start.x;
	float d = l.start.y - l.slope * l.start.x;
	float x = (d - c) / (slope - l.slope);
	if (isBetween(x, start.x, end.x) && isBetween(x, l.start.x, l.end.x)) {
		where = Point(x, slope * x + c);
		return true;
	}
	return false;
}

void Line::sanity_check() const {
	if (type == NORMAL) {
		double xp = pointAtY(yIntercept()).x;
		double yp = pointAtX(xIntercept()).y;
		assert(fabs(xp) < 1);
		assert(fabs(yp) < 1);
	}
}

/* vim: set noet: */
