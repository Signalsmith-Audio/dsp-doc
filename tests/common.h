// Custom style if available
#if defined(__has_include) && __has_include("plot-style.h")
#	include "plot-style.h"
#else
#	include "plot.h"
#endif

using Figure = signalsmith::plot::Figure;
using Plot2D = signalsmith::plot::Plot2D;

#include "../util/csv-writer.h"

