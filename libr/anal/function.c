/* radare - LGPL - Copyright 2019 - pancake */

#include <r_anal.h>

#define D if (anal->verbose)

R_API const RList *r_anal_get_functions(RAnal *anal, ut64 addr) {
	RAnalBlock *bb = r_anal_get_block_in (anal, addr);
	// TODO: this might be a bit dangerous. If the bb gets unrefd, the list must not be touched anymore.
	return bb? bb->fcns: NULL;
}

R_API char *r_anal_new_name(RAnal *anal, const char *name) {
	// check if name exists, if so suggest a new name
	return NULL;
}

static bool __fcn_exists(RAnal *anal, const char *name, ut64 addr) {
	// check if name is already registered
	bool found = false;
	if (addr == UT64_MAX) {
		eprintf ("Invalid function address (-1) '%s'\n", name);
		return true;
	}
	if (!name) {
		eprintf ("TODO: Empty function name, we must auto generate one\n");
		return true;
	}
	RAnalFunction *f = ht_pp_find (anal->ht_name_fun, name, &found);
	if (f && found) {
		eprintf ("Invalid function name '%s' at 0x%08"PFMT64x"\n", name, addr);
		return true;
	}
	// check if there's a function already in the given address
	found = false;
	f = ht_up_find (anal->ht_addr_fun, addr, &found);
	if (f && found) {
		eprintf ("Function already defined in 0x%08"PFMT64x"\n", addr);
		return true;
	}
	return false;
}

R_API RAnalFunction *r_anal_get_function_at(RAnal *anal, ut64 addr) {
	bool found = false;
	RAnalFunction *f = ht_up_find (anal->ht_addr_fun, addr, &found);
	if (f && found) {
		return f;
	}
	return NULL;
}

R_API bool r_anal_add_function_ll(RAnal *anal, RAnalFunction *fcn) {
	if (__fcn_exists (anal, fcn->name, fcn->addr)) {
		return false;
	}
	r_anal_fcn_tree_insert (anal, fcn);
	r_list_append (anal->fcns, fcn);
	ht_pp_insert (anal->ht_name_fun, fcn->name, fcn);
	ht_up_insert (anal->ht_addr_fun, fcn->addr, fcn);
	return true;
}

R_API RAnalFunction *r_anal_add_function(RAnal *anal, const char *name, ut64 addr) {
	RAnalFunction *fcn = r_anal_fcn_new (anal);
	if (fcn) {
		fcn->addr = addr;
		if (name) {
			free (fcn->name);
			fcn->name = strdup (name);
		}
		if (r_anal_add_function_ll (anal, fcn)) {
			return fcn;
		}
		r_anal_fcn_free (fcn);
	}
	return NULL;
}

R_API void r_anal_function_block_add(RAnalFunction *fcn, RAnalBlock *bb) {
	if (r_list_contains (fcn->bbs, bb)) { // TODO: might be slow, use bb->fcns instead?
		return;
	}
	r_list_append (bb->fcns, fcn); // associate the given fcn with this bb
	r_anal_block_ref (bb);
	r_list_append (fcn->bbs, bb);
	if (fcn->anal->cb.on_fcn_bb_new) {
		fcn->anal->cb.on_fcn_bb_new (fcn->anal, fcn->anal->user, fcn, bb);
	}
}

R_API void r_anal_function_block_remove(RAnalFunction *fcn, RAnalBlock *bb) {
	r_list_delete_data (bb->fcns, fcn);
	r_list_delete_data (fcn->bbs, bb);
	r_anal_block_unref (bb);

	// TODO: this might be quite heavy all the time
	ut64 size = 0;
	RAnalBlock *block;
	RListIter *iter;
	r_list_foreach (fcn->bbs, iter, block) {
		ut64 d = block->addr + block->size - fcn->addr;
		if (d > size) {
			size = d;
		}
	}
	r_anal_fcn_set_size (fcn->anal, fcn, size);
	r_anal_fcn_update_tinyrange_bbs (fcn);
}

R_API bool r_anal_del_function(RAnalFunction *fcn) {
	RAnal *anal = fcn->anal;
	eprintf ("del fun\n");
	ht_up_delete (anal->ht_addr_fun, fcn->addr);
	ht_pp_delete (anal->ht_name_fun, fcn->name);
	r_list_delete_data (anal->fcns, fcn);
	r_anal_fcn_tree_delete (anal, fcn);
	if (!r_anal_fcn_tree_delete (anal, fcn)) {
		return false;
	}
	r_list_free (fcn->bbs);
	fcn->bbs = NULL;
#if 0
	RListIter *iter, *iter2;
	RAnalBlock *bb;
	r_list_foreach_safe (fcn->bbs, iter, iter2, bb) {
		r_list_free (bb->fcns);
		//r_anal_block_unref (bb);
		r_list_delete (fcn->bbs, iter);
	}
#endif
	//r_list_delete (a->fcns, iter);
	r_list_delete_data (anal->fcns, fcn);
	//ht_up_delete (anal->ht_bbs, fcn->addr);
	D eprintf ("delete data\n");
	r_anal_fcn_free (fcn);
	r_list_free (fcn->bbs);
	r_anal_fcn_tree_delete (anal, fcn);
	return true;
}

#if 0

R_API bool r_anal_function_blocks_foreach(RAnalFunction *fcn, RAnalBlockCb cb, void *user) {
	RAnalBlock *entry = r_anal_get_block_at (fcn->anal, fcn->addr);
	if (!entry) {
		return true;
	}
	return r_anal_block_recurse (entry, cb, user);
}

R_API RList *r_anal_function_blocks_list(RAnalFunction *fcn) {
	RAnalBlock *entry = r_anal_get_block_at (fcn->anal, fcn->addr);
	if (!entry) {
		return r_list_newf ((RListFree)r_anal_block_unref);
	}
	return r_anal_block_recurse_list (entry);
}
#endif