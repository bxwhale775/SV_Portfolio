#pragma once
#include <Windows.h>
#include <list>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <map>

#define SECTOR_H  50
#define SECTOR_W  50

class SectorMgr {
private:
	int sectorarray[SECTOR_H][SECTOR_W];
	std::unordered_set<NID>* SectorSessSet = nullptr;
	SRWLOCK* SectorlockArr = nullptr;

public:
	SectorMgr() {
		int i = 0;
		for (int h = 0; h < SECTOR_H; h++) {
			for (int w = 0; w < SECTOR_W; w++) {
				sectorarray[h][w] = i;
				i++;
			}
		}
		SectorSessSet = new std::unordered_set<NID>[i];
		SectorlockArr = new SRWLOCK[i];
		for (int locki = 0; locki < i; locki++) {
			InitializeSRWLock(&SectorlockArr[locki]);
		}

		return;
	}
	~SectorMgr() {
		delete[] SectorSessSet;
		delete[] SectorlockArr;
		return;
	}

	void findSectorPosBySectorID(int sectorID, int* h, int* w) {
		*h = sectorID / SECTOR_W;
		*w = sectorID % SECTOR_W;
		return;
	}

	int findSectorIDBySectorPos(int h, int w) {
		return sectorarray[h][w];
	}

	bool findAroundSectorBySectorID(int sectorid, std::vector<int>* sectorIDlist) {
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

		AcquireSRWLockShared(&SectorlockArr[sectorID]);
		for (auto i : SectorSessSet[sectorID]) {
			sessIDlist->push_back(i);
		}
		ReleaseSRWLockShared(&SectorlockArr[sectorID]);

		return true;
	}

	bool assignNewSectorIDtoSession(Session* sess, int sectorID) {
		bool isNewSess = false;
		if (sectorID < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sectorID) { return false; }
		if (sess->sector == sectorID) { return true; }
		if (sess->sector < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sess->sector) { isNewSess = true; }

		if (!isNewSess) {
			//lock sector in order by sector id
			int cursec = sess->sector;
			int firstsec, secondsec;
			if (cursec < sectorID) {
				firstsec = cursec;
				secondsec = sectorID;
			}
			else {
				firstsec = sectorID;
				secondsec = cursec;
			}
			AcquireSRWLockExclusive(&SectorlockArr[firstsec]);
			AcquireSRWLockExclusive(&SectorlockArr[secondsec]);
			SectorSessSet[cursec].erase(sess->uid);
			SectorSessSet[sectorID].insert(sess->uid);
			sess->sector = sectorID;
			ReleaseSRWLockExclusive(&SectorlockArr[secondsec]);
			ReleaseSRWLockExclusive(&SectorlockArr[firstsec]);
		}
		else {
			AcquireSRWLockExclusive(&SectorlockArr[sectorID]);
			SectorSessSet[sectorID].insert(sess->uid);
			sess->sector = sectorID;
			ReleaseSRWLockExclusive(&SectorlockArr[sectorID]);
		}
		return true;
	}

	void deleteSessionFromSector(Session* sess) {
		if (sess->sector < 0 || sectorarray[SECTOR_H - 1][SECTOR_W - 1] < sess->sector) { return; }
		int cursec = sess->sector;
		AcquireSRWLockExclusive(&SectorlockArr[cursec]);
		SectorSessSet[cursec].erase(sess->uid);
		ReleaseSRWLockExclusive(&SectorlockArr[cursec]);
		return;
	}


	void getSessionsFromAroundSector(int sectorID, std::vector<NID>* sessIDlist) {
		std::vector<int> _vecAroundSector;
		findAroundSectorBySectorID(sectorID, &_vecAroundSector);
		for (auto& i : _vecAroundSector) {
			findSessionsBySectorID(i, sessIDlist);
		}
		return;
	}


	int getRandomSectorID() {
		return rand() / sectorarray[SECTOR_H - 1][SECTOR_W - 1];
	}
};