#include "LockOrder.h"

namespace bt
{
    // thread_local 변수 정의
    thread_local int LockOrderTracker::last_locked_order_ = 0;
} // namespace bt
