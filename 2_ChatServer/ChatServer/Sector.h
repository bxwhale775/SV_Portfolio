#pragma once
#include <list>
#include <vector>
#include <algorithm>
#include <map>

#define SECTOR_H  50
#define SECTOR_W  100

class SectorMgr {
private:
	int sectorarray[SECTOR_H][SECTOR_W];
	std::multimap<int, NID> sessionmap;

public:
	SectorMgr() {
		int i = 0;
		for (int h = 0; h < SECTOR_H; h++) {
			for (int w = 0; w < SECTOR_W; w++) {
				sectorarray[h][w] = i;
				i++;
			}
		}
	}

	void findSectorPosBySectorID(int sectorID, int* h, int* w) {
		*h = sectorID / SECTOR_W;
		*w = sectorID % SECTOR_W;
		return;
	}

	int findSectorIDBySectorPos(int h, int w) {
		return sectorarray[h][w];
	}

	bool findAroundSectorBySectorID(int sectorid, std::list<int>* sectorIDlist) {
		if (sectorid < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sectorid) { return false; }	//fail check

		sectorIDlist->push_back(sectorid);		//center

		int h, w;
		findSectorPosBySectorID(sectorid, &h, &w);

		if (!(h - 1 < 0 || w - 1 < 0)) {			//UL
			sectorIDlist->push_back(sectorarray[h - 1][w - 1]);
		}
		if (!(h - 1 < 0)) {							//UU
			sectorIDlist->push_back(sectorarray[h - 1][w]);
		}
		if (!(h - 1 < 0 || SECTOR_W <= w + 1)) {	//UR
			sectorIDlist->push_back(sectorarray[h - 1][w + 1]);
		}
		if (!(w - 1 < 0)) {							//LL
			sectorIDlist->push_back(sectorarray[h][w - 1]);
		}
		if (!(SECTOR_W <= w + 1)) {					//RR
			sectorIDlist->push_back(sectorarray[h][w + 1]);
		}
		if (!(SECTOR_H <= h + 1 || w - 1 < 0)) {	//DL
			sectorIDlist->push_back(sectorarray[h + 1][w - 1]);
		}
		if (!(SECTOR_H <= h + 1)) {					//DD
			sectorIDlist->push_back(sectorarray[h + 1][w]);
		}
		if (!(SECTOR_H <= h + 1 || SECTOR_W <= w + 1)) {	//DR
			sectorIDlist->push_back(sectorarray[h + 1][w + 1]);
		}

		return true;
	}

	bool findSessionsBySectorID(int sectorID, std::vector<NID>* sessIDlist) {
		if (sectorID < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sectorID) { return false; }	//fail check

		for (auto i = sessionmap.lower_bound(sectorID); i != sessionmap.upper_bound(sectorID); ++i) {
			sessIDlist->push_back(i->second);
		}
		return true;
	}

	bool assignNewSectorIDtoSession(Session* sess, int sectorID) {
		bool isNewSess = false;
		if (sectorID < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sectorID) { return false; }
		if (sess->sector < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sess->sector) { isNewSess = true; }

		if (!isNewSess) {
			auto range = sessionmap.equal_range(sess->sector);
			for (auto& i = range.first; i != range.second; ++i) {
				if (i->second == sess->uid) {
					sessionmap.erase(i);
					break;
				}
			}
			sess->sector = sectorID;
			sessionmap.insert({ sectorID, sess->uid });
		}
		else {
			sess->sector = sectorID;
			sessionmap.insert({ sectorID, sess->uid });
		}
		return true;
	}

	void deleteSessionFromSector(Session* sess) {
		auto range = sessionmap.equal_range(sess->sector);
		for (auto& i = range.first; i != range.second; ++i) {
			if (i->second == sess->uid) {
				sessionmap.erase(i);
				break;
			}
		}
	}

	void getSessionsFromAroundSector(int sectorID, std::vector<NID>* sessIDlist) {
		std::list<int> AroundSectorList;
		findAroundSectorBySectorID(sectorID, &AroundSectorList);
		for (auto& i : AroundSectorList) {
			findSessionsBySectorID(i, sessIDlist);
		}
		return;
	}

	int getRandomSessionID() {
		return rand() / sectorarray[SECTOR_H - 1][SECTOR_W - 1];
	}
};