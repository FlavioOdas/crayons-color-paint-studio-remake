#pragma once

#include <string>

#include "gamefiles.h"

namespace help {

// Converts the player's CRAYONS.HLP to HTML in the selected data folder.
// Returns the path to index.html, or an empty path with a diagnostic.
std::wstring exportHtml(const gamefiles::Store& files, std::wstring* error);

}
