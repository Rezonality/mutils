#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define MUtilsPlot(a, b) TracyPlot(a, b)
#define MUtilsLockable(a, b) TracyLockable(a, b)
#define MUtilsLockableBase(a) LockableBase(a)
#define MUtilsZoneScoped ZoneScoped
#define MUtilsZoneScopedN(a) ZoneScopedN(a)
#define MUtilsFrameMark FrameMark
#else
#define MUtilsPlot(a, b)
#define MUtilsLockable(a, b) a b
#define MUtilsLockableBase(a) a
#define MUtilsZoneScoped
#define MUtilsZoneScopedN(a)
#define MUtilsFrameMark
#endif
