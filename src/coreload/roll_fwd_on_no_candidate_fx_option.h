#ifndef ROLL_FWD_ON_NO_CANDIDATE_FX_OPTION_H_
#define ROLL_FWD_ON_NO_CANDIDATE_FX_OPTION_H_

namespace coreload
{
    // Specifies the roll forward capability for finding the closest (most compatible) framework
    // Note that the "applyPatches" bool option is separate from this and occurs after roll forward.
    enum class roll_fwd_on_no_candidate_fx_option
    {
        disabled = 0,
        minor,          // also inludes patch
        major           // also inludes minor and patch
    };

} // namespace coreload

#endif // ROLL_FWD_ON_NO_CANDIDATE_FX_OPTION_H_