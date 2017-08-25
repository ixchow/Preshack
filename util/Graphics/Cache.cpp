#include "Cache.hpp"
#include <iostream>

using std::cerr;
using std::endl;
using std::pair;

namespace Graphics {

CachableItem::CachableItem(unsigned int _id) : refcount(0), id(_id), loaded(true) {
}

CachableItem::~CachableItem() {
}

void Cache::add_item(string const &key, CachableItem *item) {
	map< string, CachableItem *>::iterator e = items.find(key);
	if (e != items.end()) {
		cerr << "Graphics::Cache key collision on '" << key << "'." << endl;
		return;
	}
	item->refcount += 1; //one reference, 'cause we're holding it in the cache.
	items.insert(make_pair(key, item));
}

CachableItem *Cache::get_item(string const &key) {
	map< string, CachableItem * >::iterator f = items.find(key);
	if (f != items.end()) return f->second;
	return NULL;
}

string const *Cache::get_key(CachableItem *item) {
	for (map< string, CachableItem * >::iterator i = items.begin(); i != items.end(); ++i) {
		if (i->second == item) {
			return &(i->first);
		}
	}
	return NULL;
}

void Cache::garbage_collect() {
	map< string, CachableItem * >::iterator i = items.begin();
	while (i != items.end()) {
		if (i->second->refcount <= 1) {
			//the reference from the cache shouldn't go away:
			assert(i->second->refcount == 1);
			map< string, CachableItem * >::iterator old = i++;
			CachableItem *item = old->second;
			items.erase(old);
			delete item;
		} else {
			++i;
		}
	}
}

void Cache::reload() {
	for (map< string, CachableItem * >::iterator i = items.begin(); i != items.end(); ++i) {
		i->second->loaded = false;
	}
	for (map< string, CachableItem * >::iterator i = items.begin(); i != items.end(); ++i) {
		i->second->reload();
		assert(i->second->loaded);
	}
}

Cache &get_cache() {
	static Cache cache;
	return cache;
}

} //namespace Graphics
