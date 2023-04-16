#pragma once
#include <unordered_map>
#include <set>
#include <vector>
#include <sstream>

class TinyRouter {
private:
	std::unordered_map<unsigned long, unsigned long> _Table;
	std::set<unsigned long, std::greater<unsigned long>> _Masks;

public:
	void AddRoute(const wchar_t* destination, const wchar_t* netmask, const wchar_t* gateway) {
		unsigned long ldest = IpToUlong(destination);
		unsigned long lmask = IpToUlong(netmask);
		unsigned long lgw = IpToUlong(gateway);
		_Masks.insert(lmask);
		_Table[ldest & lmask] = lgw;
		return;
	}
	void Route(const wchar_t* in_destination, wchar_t* out_gateway) {
		unsigned long ldest = IpToUlong(in_destination);
		for (auto i : _Masks) {
			auto itr_t = _Table.find(ldest & i);
			if (itr_t != _Table.end()) {
				UlongToIp(itr_t->second, out_gateway);
				return;
			}
		}
		UlongToIp(0, out_gateway);
		return;
	}
private:
	unsigned long IpToUlong(const wchar_t* ip) {
		std::vector<std::wstring> tokenvec;
		std::wstringstream ss(ip);
		std::wstring token;

		while (getline(ss, token, L'.')) {
			tokenvec.push_back(token);
		}
		if (tokenvec.size() != 4) { return 0; }

		unsigned long res = 0;
		for (int i = 0; i < 4; i++) {
			int octet = std::stoi(tokenvec[i]);
			if (octet < 0 || octet > 255) { return 0; }
			res |= octet << ((3 - i) * 8);
		}

		return res;
	}
	void UlongToIp(unsigned long in_ip, wchar_t* out_ip) {
		std::wstring res;
		for (int i = 0; i < 4; i++) {
			res = std::to_wstring((in_ip >> (i * 8)) & 0xFF) + L"." + res;
		}
		// Remove trailing '.'
		res.pop_back();

		wcscpy_s(out_ip, 16, res.c_str());
		return;
	}
};