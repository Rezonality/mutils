#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define MUtilsPlot(a, b) TracyPlot(a, b)
#define MUtilsLockable(a, b) TracyLockable(a, b)
#define MUtilsSharedLockable(a, b) TracySharedLockable(a, b)
#define MUtilsLockableBase(a) LockableBase(a)
#define MUtilsSharedLockableBase(a) SharedLockableBase(a)
#define MUtilsZoneScoped ZoneScoped
#define MUtilsZoneScopedN(a) ZoneScopedN(a)
#define MUtilsFrameMark FrameMark
#define MUtilsNameThread(a) tracy::SetThreadName(a)
#else
#define MUtilsPlot(a, b)
#define MUtilsLockable(a, b) a b
#define MUtilsSharedLockable(a, b) a b
#define MUtilsLockableBase(a) a
#define MUtilsSharedLockableBase(a) a
#define MUtilsZoneScoped
#define MUtilsZoneScopedN(a)
#define MUtilsFrameMark
#define MUtilsNameThread(a)
#endif
