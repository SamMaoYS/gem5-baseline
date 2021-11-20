#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_DBP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_DBP_HH__

#include "mem/cache/replacement_policies/base.hh"

struct DBPParams;

class DBP : public BaseReplacementPolicy
{
  protected:
    struct DeadBlockData : ReplacementData
    {
        bool dead;

        /**
         * Default constructor. Invalidate data.
         */
        DeadBlockData() : dead(false) {}
    };

  public:
    /** Convenience typedef. */
    typedef DBPParams Params;

    /**
     * Construct and initiliaze this replacement policy.
     */
    DBP(const Params *p);

    /**
     * Destructor.
     */
    ~DBP() {}

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Prioritize replacement data for victimization.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                               const override;

    /**
     * Touch an entry to update its replacement data.
     * Does not do anything.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Unprioritize replacement data for victimization.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim at random.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_DBP_HH__
