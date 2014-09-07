#include "predictors.h"

bool always_taken::predict(pc_t, bool taken)
{
	return taken;
}

bool always_not_taken::predict(pc_t, bool taken)
{
	return !taken;
}

tournament::tournament():
	selector_table(all_ones)
{
}

bool tournament::predict(pc_t pc, bool taken)
{
	auto counter = selector_table[pc];
	auto which_predictor = counter.mode();
	bool bimodal_correct = bimodal_pred.predict(pc, taken);
	bool gshare_correct = gshare_pred.predict(pc, taken);
	if (bimodal_correct != gshare_correct)
	{
		if (bimodal_correct)
			++counter;
		else
			--counter;
	}
	return which_predictor? bimodal_correct : gshare_correct;
}

constexpr yags::cache::entry::entry(bool b):
	tag(), counter(b? MAX_COUNTER : 0)
{
}

constexpr bool yags::cache::entry::hit(pc_t pc) const
{
	return tag == (pc & TAG_BITMASK);
}

constexpr bool yags::cache::entry::mode() const
{
	return counter & COUNTER_MODE_BITMASK;
}

void yags::cache::entry::set_tag(pc_t pc)
{
	tag = pc & TAG_BITMASK;
}

void yags::cache::entry::operator++()
{
	if (counter < MAX_COUNTER)
		++counter;
}

void yags::cache::entry::operator--()
{
	if (counter > 0)
		--counter;
}

yags::cache::cache(mode_1_t)
{
	entries.fill(entry(true));
}

auto yags::cache::operator[](pc_t pc) -> ref
{
	return entries[pc % NUM_ENTRIES];
}

auto yags::cache::operator[](pc_t pc) const -> cref
{
	return entries[pc % NUM_ENTRIES];
}

yags::yags():
	taken_cache(cache::mode_1)
{
}

bool yags::predict(pc_t pc, bool taken)
{
	auto selector_counter = selector_table[pc];
	auto choice = selector_counter.mode();
	/* Use the opposite cache to find the special case when a branch has gone against its bias.
	 * Also use it to update when you discover this special case:
	 * get a cache miss and use the choice direction but that turns out to be wrong.  */
	auto &chosen_entry = (choice? not_taken_cache : taken_cache)[pc ^ global_history];
	bool hit = chosen_entry.hit(pc);
	bool correct = (hit? chosen_entry.mode() : choice) == taken;
	// miss and incorrect implies only the choice table was used and it was wrong
	if (hit || !correct) 
	{
		if (taken)
			++chosen_entry;
		else
			--chosen_entry;
		chosen_entry.set_tag(pc);
	}
	/* Same as bimode:
	 * the sole exception to updating the selector counter
	 * is when the choice is "wrong" but the chosen counter is right anyway. */
	if (!((choice != taken) && correct))
	{
		if (taken)
			++selector_counter;
		else
			--selector_counter;
	}
	global_history << taken;
	return correct;
}
