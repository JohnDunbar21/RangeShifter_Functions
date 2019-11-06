//---------------------------------------------------------------------------

#pragma hdrstop

#include "Landscape.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

ifstream landscape;

ofstream outConnMat;
#if HEATMAP
ofstream outvisits;
#endif

//---------------------------------------------------------------------------

// Initial species distribution functions

InitDist::InitDist(Species *pSp)
{
pSpecies = pSp;
}

InitDist::~InitDist() {
int ncells = cells.size();
for (int i = 0; i < ncells; i++)
	if (cells[i] != NULL) delete cells[i];
cells.clear();
}

void InitDist::setDistribution(int nInit) {
int rr;
int ncells = (int)cells.size();
if (nInit == 0) { // set all cells to be initialised
	for (int i = 0; i < ncells; i++) {
		cells[i]->setCell(true);
	}
}
else { // set specified number of cells at random to be initialised
	if (nInit > ncells/2) { // use backwards selection method
		for (int i = 0; i < ncells; i++) cells[i]->setCell(true);
		for (int i = 0; i < (ncells-nInit); i++) {
			do {
				rr = pRandom->IRandom(0,ncells-1);
			} while (!cells[rr]->selected());
			cells[rr]->setCell(false);
		}
	}
	else { // use forwards selection method
		for (int i = 0; i < ncells; i++) cells[i]->setCell(false);
		for (int i = 0; i < nInit; i++) {
			do {
				rr = pRandom->IRandom(0,ncells-1);
			} while (cells[rr]->selected());
			cells[rr]->setCell(true);
		}
	}
}
}

// Set a specified cell (by position in cells vector)
void InitDist::setDistCell(int ix,bool init) {
cells[ix]->setCell(init);
}

// Set a specified cell (by co-ordinates)
void InitDist::setDistCell(locn loc,bool init) {
locn cellloc;
int ncells = (int)cells.size();
for (int i = 0; i < ncells; i++) {
	cellloc = cells[i]->getLocn();
	if (cellloc.x == loc.x && cellloc.y == loc.y) {
		cells[i]->setCell(init);
		i = ncells;
	}
}
}

// Specified location is within the initial distribution?
bool InitDist::inInitialDist(locn loc) {
int ncells = (int)cells.size();
for (int i = 0; i < ncells; i++) {
	if (cells[i]->toInitialise(loc)) { // cell is to be initialised
		return true;
	}
}
return false;
}

int InitDist::cellCount(void) {
return (int)cells.size();
}

// Return the co-ordinates of a specified initial distribution cell
locn InitDist::getCell(int ix) {
locn loc;
if (ix >= 0 && ix < (int)cells.size()) {
	loc = cells[ix]->getLocn();
}
else {
	loc.x = loc.y = -666; // indicates invalid index specified
}
return loc;
}

// Return the co-ordinates of a specified initial distribution cell if it has been
// selected - otherwise return negative co-ordinates
locn InitDist::getSelectedCell(int ix) {
locn loc; loc.x = loc.y = -666;
if (ix < (int)cells.size()) {
	if (cells[ix]->selected()) {
		loc = cells[ix]->getLocn();
	}
}
return loc;
}

locn InitDist::getDimensions(void) {
locn d; d.x = maxX; d.y = maxY; return d;
}

void InitDist::resetDistribution(void) {
int ncells = (int)cells.size();
for (int i = 0; i < ncells; i++) {
	cells[i]->setCell(false);
}
}

//---------------------------------------------------------------------------

// Read species initial distribution file

int InitDist::readDistribution(string distfile) {
string header;
int p,nodata;
int ncols,nrows;
ifstream dfile; // species distribution file input stream

// open distribution file
dfile.open(distfile.c_str());
if (!dfile.is_open()) return 21;

// read landscape data from header records of distribution file
// NB headers of all files have already been compared
dfile >> header >> ncols >> header >> nrows >> header >> minEast >> header >> minNorth
	>> header >> resol >> header >> nodata;
maxX = ncols-1; maxY = nrows-1;

// set up bad integer value to ensure that valid values are read
int badvalue = -9; if (nodata == -9) badvalue = -99;

for (int y = nrows-1; y >= 0; y--) {
	for (int x = 0; x < ncols; x++) {
		p = badvalue; dfile >> p;
#if RSDEBUG
//DEBUGLOG << "InitDist::readDistribution():"
//	<< " y = " << y << " x = " << x << " p = " << p << endl;
#endif
		if (p == nodata || p == 0 || p == 1) { // only valid values
			if (p == 1) { // species present
				cells.push_back(new DistCell(x,y));
			}
		}
		else { // error in file
			dfile.close(); dfile.clear();
			return 22;
		}
	}
}

dfile.close(); dfile.clear();
return 0;
}


//---------------------------------------------------------------------------

// Landscape functions

Landscape::Landscape(void) {
patchModel = false; spDist = false; generated = false; fractal = false; continuous = false;
dynamic = false; habIndexed = false;
#if RS_CONTAIN
dmgLoaded = false;
#endif // RS_CONTAIN 
resol = spResol = landNum = 0;
rasterType = 0;
nHab = nHabMax = 0;
dimX = dimY = 100;
minX = minY = 0;
maxX = maxY = 99;
minPct = maxPct = propSuit = hurst = 0.0;
maxCells = 100;
pix = gpix = 1.0;
minEast = minNorth = 0.0;
cells = 0;
#if RSDEBUG
// NOTE: do NOT write to output stream before it has been opened - it will be empty
//DebugGUI("Landscape::Landscape(): this = " + Int2Str((int)this)
//	+ " pCell = " + Int2Str((int)pCell));
//DEBUGLOGGUI << "Landscape::Landscape(): this = " << this << " pCell = " << pCell << endl;
#endif
connectMatrix = 0;
epsGlobal = 0;
patchChgMatrix = 0;
#if RS_CONTAIN
alpha = 1.0;					
#endif // RS_CONTAIN 

#if RSDEBUG
//DEBUGLOG << "Landscape::Landscape():"
//	<< " rasterType= " << rasterType << endl;
#endif
#if RSDEBUG
//MemoLine(("Landscape::Landscape(): landscape created " + Int2Str(0)
//	).c_str());
#endif
}

Landscape::~Landscape() {

#if RSDEBUG
//DebugGUI(("Landscape::~Landscape(): this=" + Int2Str((int)this) + " cells=" + Int2Str((int)cells)
//	 + " maxX=" + Int2Str(maxX) + " maxY=" + Int2Str(maxY)).c_str());
#endif
if (cells != 0) {
	for (int y = dimY-1; y >= 0; y--) {
#if RSDEBUG
//DebugGUI(("Landscape::~Landscape(): y=" + Int2Str(y) + " cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
		for (int x = 0; x < dimX; x++) {
#if RSDEBUG
//DebugGUI(("Landscape::~Landscape(): y=" + Int2Str(y) + " x=" + Int2Str(x)
//	+ " cells[y][x]=" + Int2Str((int)cells[y][x])).c_str());
#endif
			if (cells[y][x] != 0) delete cells[y][x];
		}
		if (cells[y] != 0) {
#if RSDEBUG
//DebugGUI(("Landscape::~Landscape(): deleting cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
//			delete cells[y];
			delete[] cells[y];
		}
	}
	delete[] cells;
	cells = 0;
}
#if RSDEBUG
//DebugGUI(("Landscape::~Landscape(): this=" + Int2Str((int)this)
//	+ " cells=" + Int2Str((int)cells)).c_str());
#endif

int npatches = patches.size();
for (int i = 0; i < npatches; i++)
	if (patches[i] != NULL) delete patches[i];
patches.clear();

int ndistns = distns.size();
for (int i = 0; i < ndistns; i++)
	if (distns[i] != NULL) delete distns[i];
distns.clear();

int ninitcells = initcells.size();
for (int i = 0; i < ninitcells; i++)
	if (initcells[i] != NULL) delete initcells[i];
initcells.clear();

habCodes.clear();
colours.clear();

deleteConnectMatrix();
deletePatchChgMatrix();

#if RS_CONTAIN
int ndlocns = dmglocns.size();
for (int i = 0; i < ndlocns; i++)
	if (dmglocns[i] != NULL) delete dmglocns[i];
dmglocns.clear();
#endif // RS_CONTAIN 

#if RSDEBUG
//MemoLine(("Landscape::~Landscape(): landscape deleted " + Int2Str(1)
//	).c_str());
#endif
}

// Remove all patches and cells
// Used for replicating artificial landscape without deleting the landscape itself
void Landscape::resetLand(void) {

#if RSDEBUG
//DebugGUI("Landscape::resetLand(): starting...");
#endif
resetLandLimits();
int npatches = patches.size();
#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): npatches=" + Int2Str(npatches)
//	).c_str());
#endif
for (int i = 0; i < npatches; i++) if (patches[i] != NULL) delete patches[i];
patches.clear();
#if RSDEBUG
//DebugGUI("Landscape::resetLand(): finished resetting patches");
#endif

#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): this=" + Int2Str((int)this)
//	+ " cells=" + Int2Str((int)cells)).c_str());
#endif
if (cells != 0) {
	for(int y = dimY-1; y >= 0; y--){
#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): y=" + Int2Str(y) + " cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
		for (int x = 0; x < dimX; x++) {
#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): y=" + Int2Str(y) + " x=" + Int2Str(x)
//	+ " cells[y][x]=" + Int2Str((int)cells[y][x])).c_str());
#endif
			if (cells[y][x] != 0) delete cells[y][x];
		}
		if (cells[y] != 0) {
#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): deleting cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
			delete[] cells[y];
//			delete cells[y];
		}
	}
	delete[] cells;
	cells = 0;
}
#if RSDEBUG
//DebugGUI(("Landscape::resetLand(): this=" + Int2Str((int)this)
//	+ " cells=" + Int2Str((int)cells)).c_str());
#endif
}

void Landscape::setLandParams(landParams ppp,bool batchMode)
{
generated = ppp.generated; patchModel = ppp.patchModel; spDist = ppp.spDist;
dynamic = ppp.dynamic;
landNum = ppp.landNum;
#if RS_CONTAIN
dmgLoaded = ppp.dmgLoaded;
#endif // RS_CONTAIN 
if (ppp.resol > 0) resol = ppp.resol;
if (ppp.spResol > 0 && ppp.spResol%ppp.resol == 0) spResol = ppp.spResol;
if ((ppp.rasterType >= 0 && ppp.rasterType <= 2) || ppp.rasterType == 9)
	rasterType = ppp.rasterType;
if (ppp.nHab >= 1) nHab = ppp.nHab;
if (ppp.nHabMax >= 1) nHabMax = ppp.nHabMax;
if (ppp.dimX > 0) dimX = ppp.dimX;
if (ppp.dimY > 0) dimY = ppp.dimY;
if (ppp.minX >= 0 && ppp.maxX >= 0 && ppp.minX <= ppp.maxX && ppp.maxX < dimX) {
	minX = ppp.minX; maxX = ppp.maxX;
}
else {
	minX = 0; maxX = dimX - 1;
}
if (ppp.minY >= 0 && ppp.maxY >= 0 && ppp.minY <= ppp.maxY && ppp.maxY < dimY) {
	minY = ppp.minY; maxY = ppp.maxY;
}
else {
	minY = 0; maxY = dimY - 1;
}
if (batchMode && rasterType == 0) {
	// in batch mode, set up sequential habitat codes if not already present
	if (habCodes.size() == 0) {
		for (int i = 0; i < nHabMax; i++) {
			habCodes.push_back(i+1);
		}
	}
}
}

landParams Landscape::getLandParams(void)
{
landParams ppp;
ppp.generated = generated; ppp.patchModel = patchModel; ppp.spDist = spDist;
ppp.dynamic = dynamic;
#if RS_CONTAIN
ppp.dmgLoaded = dmgLoaded;
#endif // RS_CONTAIN 
ppp.landNum = landNum;
ppp.resol = resol; ppp.spResol = spResol;
ppp.rasterType = rasterType;
ppp.nHab = nHab; ppp.nHabMax = nHabMax;
ppp.dimX = dimX; ppp.dimY = dimY;
ppp.minX = minX; ppp.minY = minY;
ppp.maxX = maxX; ppp.maxY = maxY;
return ppp;
}

landData Landscape::getLandData(void) {
landData dd;
dd.resol = resol;
dd.dimX = dimX; dd.dimY = dimY;
dd.minX = minX; dd.minY = minY;
dd.maxX = maxX; dd.maxY = maxY;
return dd;
}

void Landscape::setGenLandParams(genLandParams ppp)
{
fractal = ppp.fractal;
continuous = ppp.continuous;
if (ppp.minPct > 0.0 && ppp.minPct < 100.0) minPct = ppp.minPct;
if (ppp.maxPct > 0.0 && ppp.maxPct <= 100.0) maxPct = ppp.maxPct;
if (ppp.propSuit >= 0.0 && ppp.propSuit <= 1.0) propSuit = ppp.propSuit;
if (ppp.hurst > 0.0 && ppp.hurst < 1.0) hurst = ppp.hurst;
if (ppp.maxCells > 0) maxCells = ppp.maxCells;
}

genLandParams Landscape::getGenLandParams(void)
{
genLandParams ppp;
ppp.fractal = fractal; ppp.continuous = continuous;
ppp.minPct = minPct; ppp.maxPct = maxPct; ppp.propSuit = propSuit; ppp.hurst = hurst;
ppp.maxCells = maxCells;
return ppp;
}

void Landscape::setLandLimits(int x0,int y0,int x1,int y1) {
if (x0 >= 0 && x1 >= 0 && x0 <= x1 && x1 < dimX
&&  y0 >= 0 && y1 >= 0 && y0 <= y1 && y1 < dimY) {
	minX = x0; maxX = x1; minY = y0; maxY = y1;
}
}

void Landscape::resetLandLimits(void) {
minX = minY = 0; maxX = dimX-1; maxY = dimY-1;
}

//---------------------------------------------------------------------------

void Landscape::setLandPix(landPix p) {
if (p.pix > 0) pix = p.pix;
if (p.gpix > 0.0) gpix = p.gpix;
}

landPix Landscape::getLandPix(void) {
landPix p;
p.pix = pix; p.gpix = gpix;
return p;
}

void Landscape::setOrigin(landOrigin origin) {
minEast = origin.minEast; minNorth = origin.minNorth;
}

landOrigin Landscape::getOrigin(void) {
landOrigin origin;
origin.minEast = minEast; origin.minNorth = minNorth;
return origin;
}

//---------------------------------------------------------------------------

// Functions to handle habitat codes

bool Landscape::habitatsIndexed(void) { return habIndexed; }

void Landscape::listHabCodes(void) {
int nhab = (int)habCodes.size();
cout << endl;
for (int i = 0; i < nhab; i++) {
	cout << "Habitat code[ " << i << "] = " << habCodes[i] << endl;
}
cout << endl;
}

void Landscape::addHabCode(int hab) {
int nhab = (int)habCodes.size();
bool addCode = true;
for (int i = 0; i < nhab; i++) {
	if (hab == habCodes[i]) {
		addCode = false; i = nhab+1;
	}
}
if (addCode) { habCodes.push_back(hab); nHab++; }
}

// Get the index number of the specified habitat in the habitats vector
int Landscape::findHabCode(int hab) {
int nhab = (int)habCodes.size();
for (int i = 0; i < nhab; i++) {
	if (hab == habCodes[i]) return i;
}
return -999;
}

// Get the specified habitat code
int Landscape::getHabCode(int ixhab) {
if (ixhab < (int)habCodes.size()) return habCodes[ixhab];
else return -999;
}

void Landscape::clearHabitats(void) {
habCodes.clear();
colours.clear();
}

void Landscape::addColour(rgb c) {
colours.push_back(c);
}

void Landscape::changeColour(int i,rgb col) {
int ncolours = (int)colours.size();
if (i >= 0 && i < ncolours) {
	if (col.r >=0 && col.r <= 255 && col.g >=0 && col.g <= 255 && col.b >=0 && col.b <= 255)
		colours[i] = col;
}
}

rgb Landscape::getColour(int ix) {
return colours[ix];
}

int Landscape::colourCount(void) {
return colours.size();
}

//---------------------------------------------------------------------------
void Landscape::setCellArray(void) {
#if RSDEBUG
//DebugGUI(("Landscape::setCellArray(): start: this=" + Int2Str((int)this)
//	+ " cells=" + Int2Str((int)cells)).c_str());
#endif
if (cells != 0) resetLand();
cells = new Cell **[maxY+1];
#if RSDEBUG
//DebugGUI(("Landscape::setCellArray(): cells=" + Int2Str((int)cells)).c_str());
#endif
for (int y = dimY-1; y >= 0; y--) {
	cells[y] = new Cell *[dimX];
#if RSDEBUG
//DebugGUI(("Landscape::setCellArray(): y=" + Int2Str(y)
//	+ " cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
	for (int x = 0; x < dimX; x++) {
		cells[y][x] = 0;
	}
}
#if RSDEBUG
//DebugGUI(("Landscape::setCellArray(): end: this=" + Int2Str((int)this)
//	+ " cells=" + Int2Str((int)cells)).c_str());
#endif
}

void Landscape::addPatchNum(int p) {
int npatches = (int)patchnums.size();
bool addpatch = true;
for (int i = 0; i < npatches; i++) {
	if (p == patchnums[i]) {
		addpatch = false; i = npatches+1;
	}
}
if (addpatch) patchnums.push_back(p);
}


//---------------------------------------------------------------------------
/* Create an artificial landscape (random or fractal), which can be
either binary (habitat index 0 is the matrix, 1 is suitable habitat)
or continuous (0 is the matrix, >0 is suitable habitat) */
void Landscape::generatePatches(void)
{
int x,y,ncells;
//float p,prop;
float p;
Patch *pPatch;
Cell *pCell;

vector <land> ArtLandscape;

#if RSDEBUG
//int iiiiii = (int)fractal;
//DEBUGLOG << "Landscape::generatePatches(): (int)fractal=" << iiiiii
//	<< " rasterType=" << rasterType
//	<< " continuous=" << continuous
//	<< " dimX=" << dimX << " dimY=" << dimY
//	<< " propSuit=" << propSuit
//	<< " hurst=" << hurst
//	<< " maxPct=" << maxPct << " minPct=" << minPct
//	<< endl;
#endif

setCellArray();

int patchnum = 0;  // initial patch number for cell-based landscape
// create patch 0 - the matrix patch (even if there is no matrix)
#if SEASONAL
newPatch(patchnum++,1);
#else
newPatch(patchnum++);
#endif // SEASONAL 

// as landscape generator returns cells in a random sequence, first set up all cells
// in the landscape in the correct sequence, then update them and create patches for
// habitat cells
for (int yy = dimY-1; yy >= 0; yy--) {
	for (int xx = 0; xx < dimX; xx++) {
#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches(): yy=" << yy	<< " xx=" << xx << endl;
#endif
		addNewCellToLand(xx,yy,0);
	}
}

if (continuous) rasterType = 2;
else rasterType = 0;
if (fractal) {
	p = 1.0 - propSuit;
	// fractal_landscape() requires Max_prop > 1 (but does not check it!)
	// as in turn it calls runif(1.0,Max_prop)
	float maxpct;
	if (maxPct < 1.0) maxpct = 100.0; else maxpct = maxPct;

	ArtLandscape = fractal_landscape(dimY,dimX,hurst,p,maxpct,minPct);

	vector<land>::iterator iter = ArtLandscape.begin();
	while (iter != ArtLandscape.end()) {
		x = iter->y_coord; y = iter->x_coord;
#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches(): x=" << x	<< " y=" << y
//	<< " iter->avail=" << iter->avail
//	<< " iter->value=" << iter->value << endl;
#endif
		pCell = findCell(x,y);
		if (continuous) {
			if (iter->value > 0.0) { // habitat
#if SEASONAL
				pPatch = newPatch(patchnum++,1);
#else
				pPatch = newPatch(patchnum++);
#endif // SEASONAL 
				addCellToPatch(pCell,pPatch,iter->value);
			}
			else { // matrix
				addCellToPatch(pCell,patches[0],iter->value);
			}
		}
		else { // discrete
			if (iter->avail == 0) { // matrix
				addCellToPatch(pCell,patches[0]);
			}
			else { // habitat
#if SEASONAL
				pPatch = newPatch(patchnum++,1);
#else
				pPatch = newPatch(patchnum++);
#endif // SEASONAL 
				addCellToPatch(pCell,pPatch);
				pCell->changeHabIndex(0,1);
			}
		}
		iter++;
	}
}
else { // random landscape
	int hab;
	ncells = (int)((float)(dimX) * (float)(dimY) * propSuit + 0.00001); // no. of cells to initialise
#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches(): dimX=" << dimX	<< " dimY=" << dimY
//	<< " propSuit=" << propSuit
//	<< " PRODUCT=" << ((float)(dimX) * (float)(dimY) * propSuit + 0.00001)
//	<< " ncells=" << ncells << endl;
#endif
	int i = 0;
	do {
		do {
			x = pRandom->IRandom(0,dimX-1); y = pRandom->IRandom(0,dimY-1);
			pCell = findCell(x,y);
			hab = pCell->getHabIndex(0);
		} while (hab > 0);
#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches() 00000: y=" << y	<< " x=" << x
//	<< " i=" << i << " patchnum=" << patchnum
//	<< endl;
#endif
#if SEASONAL
		pPatch = newPatch(patchnum++,1);
#else
		pPatch = newPatch(patchnum++);
#endif // SEASONAL 
		pCell = findCell(x,y);
		if (continuous) {
			addCellToPatch(pCell,pPatch);
			pCell->setHabitat(minPct + pRandom->Random() * (maxPct - minPct));
		}
		else { // discrete
			addCellToPatch(pCell,pPatch);
			pCell->changeHabIndex(0,1);
		}
		i++;
	} while (i < ncells);
	// remaining cells need to be added to the matrix patch
	p = 0.0;
	x = 0;
	for (int yy = dimY-1; yy >= 0; yy--) {
		for (int xx = 0; xx < dimX; xx++) {
			pCell = findCell(xx,yy);
			if (continuous) {
//				if (pCell->getQuality() <= 0.0)
				if (pCell->getHabitat(0) <= 0.0)
				{
					addCellToPatch(pCell,patches[0],p);
				}
			}
			else { // discrete
				if (pCell->getHabIndex(0) == 0) {
#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches() 11111: yy=" << yy	<< " xx=" << xx
//	<< endl;
#endif
					addCellToPatch(pCell,patches[0],x);
				}
			}
		}
	}
}

#if RSDEBUG
//DEBUGLOG << "Landscape::generatePatches(): finished, no. of patches = "
//	<< patchCount() << endl;
#endif

}

//---------------------------------------------------------------------------

// Landscape patch-management functions

//---------------------------------------------------------------------------
/* Create a patch for each suitable cell of a cell-based landscape (all other
habitat cells are added to the matrix patch) */
#if SEASONAL
void Landscape::allocatePatches(Species *pSpecies,short nseasons)
#else
void Landscape::allocatePatches(Species *pSpecies)
#endif // SEASONAL 
{
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): pSpecies=" << pSpecies
//	<< " N patches=" << (int)patches.size() << endl;
#endif
//int hx;
float habK;
Patch *pPatch;
Cell *pCell;

// delete all existing patches
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): i=" << i
//	<< " patches[i]=" << patches[i] << endl;
#endif
	if (patches[i] != NULL) delete patches[i];
}
patches.clear();
// create the matrix patch
#if SEASONAL
patches.push_back(new Patch(0,0,nseasons));
#else
patches.push_back(new Patch(0,0));
#endif // SEASONAL 
Patch *matrixPatch = patches[0];
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): npatches=" << npatches
//	<< " N patches=" << (int)patches.size() << endl;
#endif
int patchnum = 1;

switch (rasterType) {

case 0: // habitat codes
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " cells=" << cells[y][x] << endl;
#endif
			if (cells[y][x] != 0) { // not no-data cell
				pCell = cells[y][x];
//				hx = pCell->getHabIndex();
//				habK = pSpecies->getHabK(hx);
				habK = 0.0;
				int nhab = pCell->nHabitats();
				for (int i = 0; i < nhab; i++) {
#if SEASONAL
					for (int j = 0; j < nseasons; j++) {
						habK += pSpecies->getHabK(pCell->getHabIndex(i),j);						
					}
#else
					habK += pSpecies->getHabK(pCell->getHabIndex(i));
#endif // SEASONAL 
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " i=" << i
//	<< " cells[y][x]=" << cells[y][x]
//	<< " pCell=" << pCell
//	<< " habK=" << habK << endl;
#endif
				}
				if (habK > 0.0) { // cell is suitable - create a patch for it
#if SEASONAL
					pPatch = newPatch(patchnum++,nseasons);
#else
					pPatch = newPatch(patchnum++);
#endif // SEASONAL 
					addCellToPatch(pCell,pPatch);
				}
				else { // cell is not suitable - add to the matrix patch
					addCellToPatch(pCell,matrixPatch);
				}
			}
		}
	}
	break;
case 1: // habitat cover
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " cells=" << cells[y][x] << endl;
#endif
			if (cells[y][x] != 0) { // not no-data cell
				pCell = cells[y][x];
				habK = 0.0;
				int nhab = pCell->nHabitats();
//				for (int i = 0; i < nHab; i++)
				for (int i = 0; i < nhab; i++)
				{
#if SEASONAL
					for (int j = 0; j < nseasons; j++) {
						habK += pSpecies->getHabK(i,j) * pCell->getHabitat(i) / 100.0;						
					}
#else
					habK += pSpecies->getHabK(i) * pCell->getHabitat(i) / 100.0;
#endif // SEASONAL 
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " i=" << i
//	<< " cells[y][x]=" << cells[y][x]
//	<< " pCell=" << pCell
//	<< " habK=" << habK << endl;
#endif
				}
				if (habK > 0.0) { // cell is suitable - create a patch for it
#if SEASONAL
					pPatch = newPatch(patchnum++,nseasons);
#else
					pPatch = newPatch(patchnum++);
#endif // SEASONAL 
					addCellToPatch(pCell,pPatch);
				}
				else { // cell is not suitable - add to the matrix patch
					addCellToPatch(pCell,matrixPatch);
				}
			}
		}
	}
	break;
case 2: // habitat quality
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " cells=" << cells[y][x] << endl;
#endif
			if (cells[y][x] != 0) { // not no-data cell
				pCell = cells[y][x];
				habK = 0.0;
				int nhab = pCell->nHabitats();
//				for (int i = 0; i < nHab; i++)
				for (int i = 0; i < nhab; i++)
				{
#if SEASONAL
					for (int j = 0; j < nseasons; j++) {
						habK += pSpecies->getHabK(0,j) * pCell->getHabitat(i) / 100.0;						
					}
#else
					habK += pSpecies->getHabK(0) * pCell->getHabitat(i) / 100.0;
#endif // SEASONAL 
#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): x=" << x << " y=" << y
//	<< " i=" << i
//	<< " cells[y][x]=" << cells[y][x]
//	<< " pCell=" << pCell
//	<< " habK=" << habK << endl;
#endif
				}
				if (habK > 0.0) { // cell is suitable (at some time) - create a patch for it
#if SEASONAL
					pPatch = newPatch(patchnum++,nseasons);
#else
					pPatch = newPatch(patchnum++);
#endif // SEASONAL 
					addCellToPatch(pCell,pPatch);
				}
				else { // cell is never suitable - add to the matrix patch
					addCellToPatch(pCell,matrixPatch);
				}
			}
		}
	}
	break;

} // end of switch (rasterType)

#if RSDEBUG
//DEBUGLOG << "Landscape::allocatePatches(): finished, N patches = " << (int)patches.size() << endl;
#endif

}

#if SEASONAL
Patch* Landscape::newPatch(int num,short nseasons) 
#else
Patch* Landscape::newPatch(int num) 
#endif // SEASONAL 
{
int npatches = (int)patches.size();
#if SEASONAL
patches.push_back(new Patch(num,num,nseasons));
#if RSDEBUG
DEBUGLOG << "Landscape::newPatch(): nseasons= " << nseasons << " num=" << num
	<< " npatches=" << (int)patches.size() << endl;
#endif
#else
patches.push_back(new Patch(num,num));
#endif // SEASONAL 
return patches[npatches];
}

#if SEASONAL
Patch* Landscape::newPatch(int seqnum,int num,short nseasons) 
#else
Patch* Landscape::newPatch(int seqnum,int num) 
#endif // SEASONAL 
{
int npatches = (int)patches.size();
#if SEASONAL
patches.push_back(new Patch(seqnum,num,nseasons));
#if RSDEBUG
DEBUGLOG << "Landscape::newPatch(): nseasons= " << nseasons 
	<< " seqnum=" << seqnum << " num=" << num
	<< " npatches=" << (int)patches.size() << endl;
#endif
#else
patches.push_back(new Patch(seqnum,num));
#endif // SEASONAL 
return patches[npatches];
}

void Landscape::resetPatches(void) {
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	patches[i]->resetLimits();
}
}

void Landscape::addNewCellToLand(int x,int y,float q) {
if (q < 0.0) // no-data cell - no Cell created
	cells[y][x] = 0;
else
	cells[y][x] = new Cell(x,y,0,q);
}

void Landscape::addNewCellToLand(int x,int y,int hab) {
if (hab < 0) // no-data cell - no Cell created
	cells[y][x] = 0;
else
	cells[y][x] = new Cell(x,y,0,hab);
}

void Landscape::addNewCellToPatch(Patch *pPatch,int x,int y,float q) {
if (q < 0.0) { // no-data cell - no Cell created
	cells[y][x] = 0;
}
else { // create the new cell
	cells[y][x] = new Cell(x,y,(intptr)pPatch,q);
	if (pPatch != 0) { // not the matrix patch
		// add the cell to the patch
		pPatch->addCell(cells[y][x],x,y);
	}
}
}

void Landscape::addNewCellToPatch(Patch *pPatch,int x,int y,int hab) {
if (hab < 0) // no-data cell - no Cell created
	cells[y][x] = 0;
else { // create the new cell
	cells[y][x] = new Cell(x,y,(intptr)pPatch,hab);
	if (pPatch != 0) { // not the matrix patch
		// add the cell to the patch
		pPatch->addCell(cells[y][x],x,y);
	}
}
}

void Landscape::addCellToPatch(Cell *pCell,Patch *pPatch) {
pCell->setPatch((intptr)pPatch);
locn loc = pCell->getLocn();
// add the cell to the patch
pPatch->addCell(pCell,loc.x,loc.y);
}

void Landscape::addCellToPatch(Cell *pCell,Patch *pPatch,float q) {
pCell->setPatch((intptr)pPatch);
// update the habitat type of the cell
pCell->setHabitat(q);
locn loc = pCell->getLocn();
// add the cell to the patch
pPatch->addCell(pCell,loc.x,loc.y);
}

void Landscape::addCellToPatch(Cell *pCell,Patch *pPatch,int hab) {
pCell->setPatch((intptr)pPatch);
// update the habitat type of the cell
pCell->setHabIndex(hab);
locn loc = pCell->getLocn();
// add the cell to the patch
pPatch->addCell(pCell,loc.x,loc.y);
}

#if RS_CONTAIN

void Landscape::setDamage(Patch *pPatch,int x,int y,int dmg) 
//void Landscape::setDamage(int x,int y,int dmg)
{
if (cells[y][x] != 0) { // not a no-data cell
	if (dmg >= 0) {
		if (pPatch == 0) { // cell is in the matrix
			cells[y][x]->setDamage(dmg);			
			dmglocns.push_back(new DamageLocn(x,y,dmg));
		}
		else { // cell is in a patch
			pPatch->setDamage(x,y,dmg);
		}
//		intptr ppatch = (intptr)pPatch;
//		dmglocns.push_back(new DamageLocn(x,y,ppatch,dmg));
	}	
}
}

void Landscape::updateDamageIndices(void) {

int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) patches[i]->resetDamageIndex();

for (int y = dimY-1; y >= 0; y--) {
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0) { // not a no-data cell
			unsigned int d = cells[y][x]->getDamage();
			if (d > 0) {
#if RSDEBUG
DEBUGLOG << "Landscape::updateDamageIndices(): y=" << y << " x=" << x
	<< " d=" << d 
	<< endl;
#endif
				for (int i = 1; i < npatches; i++) { // all except matrix patch
					patches[i]->updateDamageIndex(x,y,d,alpha);
				}
			}
		}
	}
}

#if RSDEBUG
for (int i = 1; i < npatches; i++) {
	locn l = patches[i]->getCentroid();
DEBUGLOG << "Landscape::updateDamageIndices(): i=" << i 
	<< " PatchNum=" << patches[i]->getPatchNum() 
	<< " x=" << l.x << " y=" << l.y << " damageIndex=" << patches[i]->getDamageIndex()
	<< endl;	
}
#endif

}

void Landscape::setAlpha(double a) { if (a > 0.0) alpha = a; }
double Landscape::getAlpha(void) { return alpha; }

void Landscape::resetDamageLocns(void) {
int ndlocns = (int)dmglocns.size();
for (int i = 0; i < ndlocns; i++)
	if (dmglocns[i] != NULL) dmglocns[i]->resetDamageLocn();
}

double Landscape::totalDamage(void) {
double totdmg = 0.0;
int ndlocns = (int)dmglocns.size();
for (int i = 0; i < ndlocns; i++)
	if (dmglocns[i] != NULL) totdmg += dmglocns[i]->getDamageIndex();
return totdmg;
}

#endif // RS_CONTAIN 

patchData Landscape::getPatchData(int ix) {
patchData ppp;
ppp.pPatch = patches[ix]; ppp.patchNum = patches[ix]->getPatchNum();
ppp.nCells = patches[ix]->getNCells();
locn randloc; randloc.x = -666; randloc.y = -666;
Cell *pCell = patches[ix]->getRandomCell();
if (pCell != 0) {
  randloc = pCell->getLocn();
}
ppp.x = randloc.x; ppp.y = randloc.y;
return ppp;
}

bool Landscape::existsPatch(int num) {
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	if (num == patches[i]->getPatchNum()) return true;
}
return false;
}

Patch* Landscape::findPatch(int num) {
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	if (num == patches[i]->getPatchNum()) return patches[i];
}
return 0;
}

void Landscape::resetPatchPopns(void) {
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	patches[i]->resetPopn();
}
}

void Landscape::updateCarryingCapacity(Species *pSpecies,int yr,short landIx) {
envGradParams grad = paramsGrad->getGradient();
bool gradK = false;
if (grad.gradient && grad.gradType == 1) gradK = true; // gradient in carrying capacity
patchLimits landlimits;
landlimits.xMin = minX; landlimits.xMax = maxX;
landlimits.yMin = minY; landlimits.yMax = maxY;
#if RSDEBUG
//DEBUGLOG << "Landscape::updateCarryingCapacity(): yr=" << yr
//	<< " xMin=" << landlimits.xMin << " yMin=" << landlimits.yMin
//	<< " xMax=" << landlimits.xMax << " yMax=" << landlimits.yMax
//	<< endl;
#endif
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	if (patches[i]->getPatchNum() != 0) { // not matrix patch
		patches[i]->setCarryingCapacity(pSpecies,landlimits,
			getGlobalStoch(yr),nHab,rasterType,landIx,gradK);
	}
}

}

Cell* Landscape::findCell(int x,int y) {
if (x >= 0 && x < dimX && y >= 0 && y < dimY) return cells[y][x];
else return 0;
}

int Landscape::patchCount(void) {
return patches.size();
}

void Landscape::listPatches(void) {
patchLimits p;
int npatches = (int)patches.size();
cout << endl;
for (int i = 0; i < npatches; i++) {
	p = patches[i]->getLimits();
	cout << "Patch " << patches[i]->getPatchNum()
		<< " xMin = " << p.xMin << " xMax = " << p.xMax
		<< " \tyMin = " << p.yMin << " yMax = " << p.yMax
		<< endl;
}
cout << endl;
}

// Check that total cover of any cell does not exceed 100%
// and identify matrix cells
int Landscape::checkTotalCover(void) {
if (rasterType != 1) return 0; // not appropriate test
int nCells = 0;
for(int y = dimY-1; y >= 0; y--) {
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0)
		{ // not a no-data cell
			float sumCover = 0.0;
			for (int i = 0; i < nHab; i++) {
				sumCover += cells[y][x]->getHabitat(i);
#if RSDEBUG
//DebugGUI(("Landscape::checkTotalCover(): y=" + Int2Str(y)
//	+ " x=" + Int2Str(x)
//	+ " i=" + Int2Str(i)
//	+ " sumCover=" + Float2Str(sumCover)
//	).c_str());
#endif
			}
			if (sumCover > 100.00001) nCells++; // decimal part to allow for floating point error
			if (sumCover <= 0.0) // cell is a matrix cell
				cells[y][x]->setHabIndex(0);
			else
				cells[y][x]->setHabIndex(1);
		}
	}
}
return nCells;
}

// Convert habitat codes stored on loading habitat codes landscape to
// sequential sorted index numbers
void Landscape::updateHabitatIndices(void) {
// sort codes
sort (habCodes.begin(),habCodes.end());
nHab = (int)habCodes.size();
// convert codes in landscape
int h;
int changes = (int)landchanges.size();
#if RSDEBUG
//DebugGUI(("Landscape::updateHabitatIndices(): nHab=" + Int2Str(nHab)
//	+ " changes=" + Int2Str(changes)
//	).c_str());
#endif
for(int y = dimY-1; y >= 0; y--){
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0) { // not a no-data cell
			for (int c = 0; c <= changes; c++) {
				h = cells[y][x]->getHabIndex(c);
#if RSDEBUG
//DebugGUI(("Landscape::updateHabitatIndices() 00000: y=" + Int2Str(y)
//	+ " x=" + Int2Str(x) + " c=" + Int2Str(c) + " h=" + Int2Str(h)
//	).c_str());
#endif
				if (h >= 0) {
					h = findHabCode(h);
#if RSDEBUG
//DebugGUI(("Landscape::updateHabitatIndices() 11111: y=" + Int2Str(y)
//	+ " x=" + Int2Str(x) + " c=" + Int2Str(c) + " h=" + Int2Str(h)
//	).c_str());
#endif
					cells[y][x]->changeHabIndex(c,h);
				}
			}
		}
	}
}
habIndexed = true;
}

#if SEASONAL
void Landscape::setEnvGradient(Species *pSpecies,short nseasons,bool initial) 
#else
void Landscape::setEnvGradient(Species *pSpecies,bool initial) 
#endif // SEASONAL 
{
float dist_from_opt,dev;
float habK;
//int hab;
double envval;
// gradient parameters
envGradParams grad = paramsGrad->getGradient(); 
#if RSDEBUG
//DEBUGLOG << "Landscape::setEnvGradient(): grad.opt_y = " << grad.opt_y
//	<< " grad.grad_inc = " << grad.grad_inc << " grad.factor " << grad.factor
//	<< endl;
#endif
for(int y = dimY-1; y >= 0; y--){
	for (int x = 0; x < dimX; x++) {
		// NB: gradient lies in range 0-1 for all types, and is applied when necessary...
		// ... implies gradient increment will be dimensionless in range 0-1 (but << 1)
		if (cells[y][x] != 0) { // not no-data cell
			habK = 0.0;
			int nhab = cells[y][x]->nHabitats();
#if SEASONAL
			for (int i = 0; i < nhab; i++) {
				for (int j = 0; j < nseasons; j++) {
					switch (rasterType) {
					case 0:
						habK += pSpecies->getHabK(cells[y][x]->getHabIndex(i),j);
						break;
					case 1:
						habK += pSpecies->getHabK(i,j) * cells[y][x]->getHabitat(i) / 100.0;
						break;
					case 2:
						habK += pSpecies->getHabK(0,j) * cells[y][x]->getHabitat(i) / 100.0;
						break;
					}
				}
			}
#else
			for (int i = 0; i < nhab; i++) {
				switch (rasterType) {
				case 0:
					habK += pSpecies->getHabK(cells[y][x]->getHabIndex(i));
					break;
				case 1:
					habK += pSpecies->getHabK(i) * cells[y][x]->getHabitat(i) / 100.0;
					break;
				case 2:
					habK += pSpecies->getHabK(0) * cells[y][x]->getHabitat(i) / 100.0;
					break;
				}
			}
#endif // SEASONAL 
#if RSDEBUG
//DEBUGLOG << "Landscape::setEnvGradient(): y=" << y << " x=" << x
//	<< " dist_from_opt=" << dist_from_opt << " rasterType=" << rasterType << " hab=" << hab
//	<< endl;
#endif
			if (habK > 0.0) { // suitable cell
				if (initial) { // set local environmental deviation
					cells[y][x]->setEnvDev(pRandom->Random()*(2.0) - 1.0);
				}
				dist_from_opt = fabs((double)grad.opt_y - (double)y);
				dev = cells[y][x]->getEnvDev();
				envval = 1.0 - dist_from_opt*grad.grad_inc + dev*grad.factor;
#if RSDEBUG
//DEBUGLOG << "Landscape::setEnvGradient(): y=" << y << " x=" << x
//	<< " dist_from_opt=" << dist_from_opt << " dev=" << dev << " p=" << p
//	<< endl;
#endif
				if (envval < 0.000001) envval = 0.0;
				if (envval > 1.0) envval = 1.0;
			}
			else envval = 0.0;
			cells[y][x]->setEnvVal(envval);
		}
	}
}

}

void Landscape::setGlobalStoch(int nyears) {
envStochParams env = paramsStoch->getStoch();
if (epsGlobal != 0) delete[] epsGlobal;
epsGlobal = new float[nyears];
epsGlobal[0] = pRandom->Normal(0.0,env.std)*sqrt(1.0-(env.ac*env.ac));
for (int i = 1; i < nyears; i++){
	epsGlobal[i] = env.ac*epsGlobal[i-1] + pRandom->Normal(0.0,env.std)*sqrt(1.0-(env.ac*env.ac));
}
}

#if BUTTERFLYDISP
void Landscape::readGlobalStoch(int nyears,string fname) {
ifstream stochfile;
string hdr0,hdr1;
int year;
float epsilon;
if (epsGlobal != 0) delete[] epsGlobal;
epsGlobal = new float[nyears];
for (int i = 0; i < nyears; i++) { epsGlobal[i] = 0.0; }
stochfile.open(fname.c_str());
if (stochfile.is_open()) {
	stochfile >> hdr0 >> hdr1;
	year = -98765;
	stochfile >> year >> epsilon;
	while (year != -98765 && year < nyears) {
		epsGlobal[year] = epsilon;
		year = -98765;
		stochfile >> year >> epsilon;
	}
	stochfile.close();
}
stochfile.clear();
}
#endif

float Landscape::getGlobalStoch(int yr) {
if (epsGlobal != 0 && yr >= 0) {
	return epsGlobal[yr];
}
else return 0.0;
}

void Landscape::updateLocalStoch(void) {
envStochParams env = paramsStoch->getStoch();
float randpart;
for(int y = dimY-1; y >= 0; y--){
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0) { // not a no-data cell
			randpart = pRandom->Normal(0.0,env.std) * sqrt(1.0-(env.ac*env.ac));
#if RSDEBUG
//DEBUGLOG << "Landscape::updateLocalStoch(): y=" << y << " x=" << x
//	<< " env.std= " << env.std << " env.ac= " << env.ac << " randpart= " << randpart
//	<< endl;
#endif
			cells[y][x]->updateEps(env.ac,randpart);
		}
	}
}

}


void Landscape::resetCosts(void) {
for(int y = dimY-1; y >= 0; y--){
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0) { // not a no-data cell
			cells[y][x]->resetCost();
		}
	}
}
}

// Create & initialise connectivity matrix
void Landscape::createPatchChgMatrix(void)
{
intptr patch;
Patch *pPatch;
Cell *pCell;
if (patchChgMatrix != 0) deletePatchChgMatrix();
patchChgMatrix = new int **[dimY];
for(int y = dimY-1; y >= 0; y--){
	patchChgMatrix[y] = new int *[dimX];
	for (int x = 0; x < dimX; x++) {
		patchChgMatrix[y][x] = new int [3];
		pCell = findCell(x,y);
		if (pCell == 0) { // no-data cell
			patchChgMatrix[y][x][0] = patchChgMatrix[y][x][1] = 0;
		}
		else {
			// record initial patch number
			patch = pCell->getPatch();
			if (patch == 0) { // matrix cell
				patchChgMatrix[y][x][0] = patchChgMatrix[y][x][1] = 0;
			}
			else {
				pPatch = (Patch*)patch;
				patchChgMatrix[y][x][0] = patchChgMatrix[y][x][1] = pPatch->getPatchNum();
			}
		}
		patchChgMatrix[y][x][2] = 0;
#if RSDEBUG
//DebugGUI(("Landscape::createPatchChgMatrix(): y=" + Int2Str(y)
//	+ " x=" + Int2Str(x)
//	+ " patchChgMatrix[y][x][0]=" + Int2Str(patchChgMatrix[y][x][0])
//	+ " [1]=" + Int2Str(patchChgMatrix[y][x][1])
//	+ " [2]=" + Int2Str(patchChgMatrix[y][x][2])
//	).c_str());
#endif
	}
}
}

void Landscape::recordPatchChanges(int landIx) {
if (patchChgMatrix == 0) return; // should not occur
patchChange chg;

for(int y = dimY-1; y >= 0; y--) {
	for (int x = 0; x < dimX; x++) {
		if (landIx == 0) { // reset to original landscape
			if (patchChgMatrix[y][x][0] != patchChgMatrix[y][x][2]) {
				// record change of patch for current cell
				chg.chgnum = 666666; chg.x = x; chg.y = y;
				chg.oldpatch = patchChgMatrix[y][x][2];
				chg.newpatch = patchChgMatrix[y][x][0];
				patchchanges.push_back(chg);
#if RSDEBUG
//DebugGUI(("Landscape::recordPatchChanges(): landIx=" + Int2Str(landIx)
//	+ " chg.chgnum=" + Int2Str(chg.chgnum)
//	+ " chg.x=" + Int2Str(chg.x)
//	+ " chg.y=" + Int2Str(chg.y)
//	+ " chg.oldpatch=" + Int2Str(chg.oldpatch)
//	+ " chg.newpatch=" + Int2Str(chg.newpatch)
//	).c_str());
#endif
			}
		}
		else { // any other change
			if (patchChgMatrix[y][x][2] != patchChgMatrix[y][x][1]) {
				// record change of patch for current cell
				chg.chgnum = landIx; chg.x = x; chg.y = y;
				chg.oldpatch = patchChgMatrix[y][x][1];
				chg.newpatch = patchChgMatrix[y][x][2];
				patchchanges.push_back(chg);
#if RSDEBUG
//DebugGUI(("Landscape::recordPatchChanges(): landIx=" + Int2Str(landIx)
//	+ " chg.chgnum=" + Int2Str(chg.chgnum)
//	+ " chg.x=" + Int2Str(chg.x)
//	+ " chg.y=" + Int2Str(chg.y)
//	+ " chg.oldpatch=" + Int2Str(chg.oldpatch)
//	+ " chg.newpatch=" + Int2Str(chg.newpatch)
//	).c_str());
#endif
			}
		}
		// reset cell for next landscape change
		patchChgMatrix[y][x][1] = patchChgMatrix[y][x][2];
	}
}

}

int Landscape::numPatchChanges(void) { return (int)patchchanges.size(); }

patchChange Landscape::getPatchChange(int i) {
patchChange c; c.chgnum = 99999999; c.x = c.y = c.oldpatch = c.newpatch = -1;
if (i >= 0 && i < (int)patchchanges.size()) c = patchchanges[i];
return c;
}

void Landscape::deletePatchChgMatrix(void) {
if (patchChgMatrix != 0) {
	for(int y = dimY-1; y >= 0; y--){
		for (int x = 0; x < dimX; x++) {
			delete[] patchChgMatrix[y][x];
		}
		delete[] patchChgMatrix[y];
	}
}
patchChgMatrix = 0;
}

//---------------------------------------------------------------------------

// Dynamic landscape functions

void Landscape::setDynamicLand(bool dyn) { dynamic = dyn; }

void Landscape::addLandChange(landChange c) {
#if RSDEBUG
//DebugGUI(("Landscape::addLandChange(): chgnum=" + Int2Str(c.chgnum)
//	+ " chgyear=" + Int2Str(c.chgyear)
//	).c_str());
#endif
landchanges.push_back(c);
}

int Landscape::numLandChanges(void) { return (int)landchanges.size(); }

landChange Landscape::getLandChange(short ix) {
landChange c; c.chgnum = c.chgyear = 0;
c.habfile = c.pchfile = "none";
int nchanges = (int)landchanges.size();
if (ix < nchanges) c = landchanges[ix];
return c;
}

void Landscape::deleteLandChanges(void) {
while (landchanges.size() > 0) landchanges.pop_back();
landchanges.clear();
}

int Landscape::readLandChange(int filenum) {

string header;
int h,p,habnodata,pchnodata,pchseq;
int ncols,nrows;
float hfloat,pfloat;
simParams sim = paramsSim->getSim();

if (filenum < 0) return 19;

if (patchModel && (rasterType == 0 || rasterType == 2)) {
	if (filenum == 0) { // first change
		createPatchChgMatrix();
	}
	pchseq = patchCount();
}

ifstream hfile; // habitat file input stream
ifstream pfile; // patch file input stream

// open habitat file and optionally also patch file
hfile.open(landchanges[filenum].habfile.c_str());
if (!hfile.is_open()) return 31;
if (patchModel) {
	pfile.open(landchanges[filenum].pchfile.c_str());
	if (!pfile.is_open()) {
		hfile.close(); hfile.clear();
		return 32;
	}
}

// read header records of habitat (and patch) file(s)
// NB headers of all files have already been compared
hfile >> header >> ncols >> header >> nrows >> header >> hfloat >> header >> hfloat
	>> header >> hfloat >> header >> habnodata;
if (patchModel) {
	for (int i = 0; i < 5; i++) pfile >> header >> pfloat;
	pfile >> header >> pchnodata;
}

// set up bad float values to ensure that valid values are read
float badhfloat = -9.0; if (habnodata == -9) badhfloat = -99.0;
float badpfloat = -9.0; if (pchnodata == -9) badpfloat = -99.0;

switch (rasterType) {

case 0: // raster with habitat codes - 100% habitat each cell
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
			hfloat = badhfloat; hfile >> hfloat; h = (int)hfloat;
			if (patchModel) {
				pfloat = badpfloat; pfile >> pfloat; p = (int)pfloat;
			}
#if RSDEBUG
//DebugGUI(("Landscape::readLandscape(): x=" + Int2Str(x) + " y=" + Int2Str(y)
//	+ " h=" + Int2Str(h) + " p=" + Int2Str(p)
//).c_str());
#endif
			if (cells[y][x] != 0) { // not a no data cell (in initial landscape)
				if (h == habnodata) { // invalid no data cell in change map
					hfile.close(); hfile.clear();
					return 36;
				}
				else {
					if (h < 0 || (sim.batchMode && (h < 1 || h > nHabMax))) {
						// invalid habitat code
						hfile.close(); hfile.clear();
						if (patchModel) { pfile.close(); pfile.clear(); }
						return 33;
					}
					else {
						addHabCode(h);
						cells[y][x]->setHabIndex(h);
					}
				}
				if (patchModel) {
					if (p < 0) { // invalid patch code
						hfile.close(); hfile.clear();
						pfile.close(); pfile.clear();
						return 34;
					}
					else {
						patchChgMatrix[y][x][2] = p;
						if (p > 0 && !existsPatch(p)) {
							addPatchNum(p);
							newPatch(pchseq++,p);
						}
					}
				}
			}
		}
	}
	break;

	case 2: // habitat quality
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
			hfloat = badhfloat; hfile >> hfloat; h = (int)hfloat;
			if (patchModel) {
				pfloat = badpfloat; pfile >> pfloat; p = (int)pfloat;
			}
#if RSDEBUG
//MemoLine(("y=" + Int2Str(y) + " x=" + Int2Str(x) + " hfloat=" + Float2Str(hfloat)
//	+ " p=" + Int2Str(p)).c_str());
#endif
			if (cells[y][x] != 0) { // not a no data cell (in initial landscape)
				if (h == habnodata) { // invalid no data cell in change map
					hfile.close(); hfile.clear();
					if (patchModel) { pfile.close(); pfile.clear(); }
					return 36;
				}
				else {
					if (hfloat < 0.0 || hfloat > 100.0) { // invalid quality score
						hfile.close(); hfile.clear();
						if (patchModel) { pfile.close(); pfile.clear(); }
						return 37;
					}
					else {
						cells[y][x]->setHabitat(hfloat);
					}
				}
				if (patchModel) {
					if (p < 0) { // invalid patch code
						hfile.close(); hfile.clear();
						pfile.close(); pfile.clear();
						return 34;
					}
					else {
						patchChgMatrix[y][x][2] = p;
						if (p > 0 && !existsPatch(p)) {
							addPatchNum(p);
							newPatch(pchseq++,p);
						}
					}
				}
			}
		}
	}
	break;

default:
	;
}

if (hfile.is_open()) { hfile.close(); hfile.clear(); }
if (pfile.is_open()) { pfile.close(); pfile.clear(); }
return 0;

}

//---------------------------------------------------------------------------

// Species distribution functions

int Landscape::newDistribution(Species *pSpecies, string distname) {
int readcode;
int ndistns = (int)distns.size();
distns.push_back(new InitDist(pSpecies));
readcode = distns[ndistns]->readDistribution(distname);
if (readcode != 0) { // error encountered
	// delete the distribution created above
	delete distns[ndistns];
	distns.pop_back();
}
return readcode;
}

void Landscape::setDistribution(Species *pSpecies,int nInit) {
// WILL NEED TO SELECT DISTRIBUTION FOR CORRECT SPECIES ...
// ... CURRENTLY IT IS THE ONLY ONE
distns[0]->setDistribution(nInit);
}

// Specified cell match one of the distribution cells to be initialised?
bool Landscape::inInitialDist(Species *pSpecies,locn loc) {
// convert landscape co-ordinates to distribution co-ordinates
locn initloc;
initloc.x = loc.x * resol / spResol;
initloc.y = loc.y * resol / spResol;
// WILL HAVE TO GET CORRECT SPECIES WHEN THERE ARE MULTIPLE SPECIES ...
bool initialise = distns[0]->inInitialDist(initloc);
return initialise;
}

void Landscape::deleteDistribution(Species *pSpecies) {
// WILL NEED TO SELECT DISTRIBUTION FOR CORRECT SPECIES ...
// ... CURRENTLY IT IS THE ONLY ONE
if (distns[0] != 0) delete distns[0];
}

// Return no. of initial distributions
int Landscape::distnCount(void) {
return distns.size();
}

int Landscape::distCellCount(int dist) {
return distns[dist]->cellCount();
}

// Set a cell in a specified initial distribution (by position in cells vector)
void Landscape::setDistnCell(int dist,int ix,bool init) {
distns[dist]->setDistCell(ix,init);
}

// Set a cell in a specified initial distribution (by given co-ordinates)
void Landscape::setDistnCell(int dist,locn loc,bool init) {
distns[dist]->setDistCell(loc,init);
}

// Get the co-ordinates of a specified cell in a specified initial distribution
locn Landscape::getDistnCell(int dist,int ix) {
return distns[dist]->getCell(ix);
}

// Get the co-ordinates of a specified cell in a specified initial distribution
// Returns negative co-ordinates if the cell is not selected
locn Landscape::getSelectedDistnCell(int dist,int ix) {
return distns[dist]->getSelectedCell(ix);
}

// Get the dimensions of a specified initial distribution
locn Landscape::getDistnDimensions(int dist) {
return distns[dist]->getDimensions();
}

// Reset the distribution for a given species so that all cells are deselected
void Landscape::resetDistribution(Species *pSp) {
// CURRENTLY WORKS FOR FIRST SPECIES ONLY ...
distns[0]->resetDistribution();
}

//---------------------------------------------------------------------------

// Initialisation cell functions

int Landscape::initCellCount(void) {
return initcells.size();
}

void Landscape::addInitCell(int x,int y) {
initcells.push_back(new DistCell(x,y));
}

locn Landscape::getInitCell(int ix) {
return initcells[ix]->getLocn();
}

void Landscape::clearInitCells(void) {
int ncells = initcells.size();
for (int i = 0; i < ncells; i++) {
	delete initcells[i];
}
initcells.clear();
}

//---------------------------------------------------------------------------

// Read landscape file(s)
// Returns error code or zero if read correctly

#if RS_CONTAIN
#if SEASONAL
int Landscape::readLandscape(int nseasons,int fileNum,string habfile,string pchfile,string dmgfile) 
#else
int Landscape::readLandscape(int fileNum,string habfile,string pchfile,string dmgfile) 
#endif // SEASONAL 
#else
#if SEASONAL
int Landscape::readLandscape(int nseasons,int fileNum,string habfile,string pchfile) 
#else
int Landscape::readLandscape(int fileNum,string habfile,string pchfile) 
#endif // SEASONAL 
#endif // RS_CONTAIN 
{
// fileNum == 0 for (first) habitat file and optional patch file
// fileNum > 0  for subsequent habitat files under the %cover option

string header;
int h,seq,p,habnodata,pchnodata;
int ncols,nrows;
float hfloat,pfloat;
#if RS_CONTAIN
int d,dmgnodata;
float dfloat;
#endif // RS_CONTAIN 
Patch *pPatch;
simParams sim = paramsSim->getSim();

if (fileNum < 0) return 19;

ifstream hfile; // habitat file input stream
ifstream pfile; // patch file input stream
#if RS_CONTAIN
ifstream dfile; // damage file input stream
#if RSDEBUG
//DEBUGLOG << "Landscape::readLandscape(): habfile=" << habfile << " pchfile=" << pchfile
//	<< " dmgfile=" << dmgfile 
//	<< endl;
#endif
bool readdamage = true;
if (dmgfile == "NULL") readdamage = false;
#endif // RS_CONTAIN 
initParams init = paramsInit->getInit();

// open habitat file and optionally also patch file
hfile.open(habfile.c_str());
if (!hfile.is_open()) return 11;
if (fileNum == 0) { 
	if (patchModel) {
		pfile.open(pchfile.c_str());
		if (!pfile.is_open()) {
			hfile.close(); hfile.clear();
			return 12;
		}
	}
#if RS_CONTAIN
	if (readdamage) {
		dfile.open(dmgfile.c_str());
		if (!dfile.is_open()) {
			hfile.close(); hfile.clear();
			return 15;
		}		
	}
#endif // RS_CONTAIN 
}

// read landscape data from header records of habitat file
// NB headers of all files have already been compared
hfile >> header >> ncols >> header >> nrows >> header >> minEast >> header >> minNorth
	>> header >> resol >> header >> habnodata;
dimX = ncols; dimY = nrows; minX = maxY = 0; maxX = dimX-1; maxY = dimY-1;
if (fileNum == 0) {
	// set initialisation limits to landscape limits
	init.minSeedX = init.minSeedY = 0;
	init.maxSeedX = maxX; init.maxSeedY = maxY;
	paramsInit->setInit(init);
}

if (fileNum == 0) {
	if (patchModel) {
		for (int i = 0; i < 5; i++) pfile >> header >> pfloat;
		pfile >> header >> pchnodata;		
	}
#if RS_CONTAIN
	if (readdamage) {
		for (int i = 0; i < 5; i++) dfile >> header >> dfloat;
		dfile >> header >> dmgnodata;		
	}
#endif // RS_CONTAIN 
}

if (fileNum == 0) setCellArray();

// set up bad float values to ensure that valid values are read
float badhfloat = -9.0; if (habnodata == -9) badhfloat = -99.0;
float badpfloat = -9.0; if (pchnodata == -9) badpfloat = -99.0;
#if RS_CONTAIN
float baddfloat = -9.0; if (dmgnodata == -9) baddfloat = -99.0;
#endif // RS_CONTAIN 

seq = 0; 	// initial sequential patch landscape
p = 0; 		// initial patch number for cell-based landscape
// create patch 0 - the matrix patch (even if there is no matrix)
if (fileNum == 0) newPatch(seq++,p++);

switch (rasterType) {

case 0: // raster with habitat codes - 100% habitat each cell
	if (fileNum > 0) return 19; // error condition - should not occur
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
			hfloat = badhfloat; hfile >> hfloat; h = (int)hfloat;
			if (patchModel) {
				pfloat = badpfloat; pfile >> pfloat; p = (int)pfloat;
			}
#if RSDEBUG
//DebugGUI(("Landscape::readLandscape(): x=" + Int2Str(x) + " y=" + Int2Str(y)
//	+ " h=" + Int2Str(h) + " p=" + Int2Str(p)
//).c_str());
#endif
			if (h == habnodata)
				addNewCellToLand(x,y,-1); // add cell only to landscape
			else {

				// THERE IS AN ANOMALY HERE - CURRENTLY HABITAT 0 IS OK FOR GUI VERSION BUT
				// NOT ALLOWED FOR BATCH VERSION (HABITATS MUST BE 1...n)
				// SHOULD WE MAKE THE TWO VERSIONS AGREE? ...

				if (h < 0 || (sim.batchMode && (h < 1 || h > nHabMax))) {
					// invalid habitat code
					hfile.close(); hfile.clear();
					if (patchModel) {
						pfile.close(); pfile.clear();
					}
					return 13;
				}
				else {
					addHabCode(h);
					if (patchModel) {
						if (p < 0) { // invalid patch code
							hfile.close(); hfile.clear();
							pfile.close(); pfile.clear();
							return 14;
						}
						if (p == 0) { // cell is in the matrix
							addNewCellToPatch(0,x,y,h);
						}
						else {
							if (existsPatch(p))
								addNewCellToPatch(findPatch(p),x,y,h);
							else {
#if SEASONAL
								pPatch = newPatch(seq++,p,nseasons);
#else
								pPatch = newPatch(seq++,p);
#endif // SEASONAL 
								addNewCellToPatch(pPatch,x,y,h);
							}
						}
					}
					else { // cell-based model
						// add cell to landscape (patches created later)
						addNewCellToLand(x,y,h);
					}
				}
			}
#if RS_CONTAIN
			if (readdamage) {
				dfloat = baddfloat; dfile >> dfloat; d = (int)dfloat;
				if (d < 0) { // invalid damage value
					hfile.close(); hfile.clear();
					if (pfile.is_open()) { pfile.close(); pfile.clear(); }
					return 16;					
				}
				if (patchModel) {
					if (p == 0) // cell is in the matrix
						setDamage(0,x,y,d);
					else
						setDamage(pPatch,x,y,d);					
				}
				else setDamage(0,x,y,d);
			}
#endif // RS_CONTAIN 
		}
	}
	break;

case 1: // multiple % cover
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
			hfloat = badhfloat; hfile >> hfloat; h = (int)hfloat;
			if (fileNum == 0) { // first habitat cover layer
				if (patchModel) {
					pfloat = badpfloat; pfile >> pfloat; p = (int)pfloat;
				}
#if RSDEBUG
//MemoLine(("y=" + Int2Str(y) + " x=" + Int2Str(x) + " hfloat=" + Float2Str(hfloat)
//	+ " p=" + Int2Str(p)).c_str());
#endif
				if (h == habnodata) {
					addNewCellToLand(x,y,-1); // add cell only to landscape
				}
				else {
					if (hfloat < 0.0 || hfloat > 100.0) { // invalid cover score
						hfile.close(); hfile.clear();
						if (patchModel) {
							pfile.close(); pfile.clear();
						}
						return 17;
					}
					else {
						if (patchModel) {
								if (p < 0) { // invalid patch code
								hfile.close(); hfile.clear();
								pfile.close(); pfile.clear();
								return 14;
							}
							if (p == 0) { // cell is in the matrix
								addNewCellToPatch(0,x,y,hfloat);
							}
							else {
								if (existsPatch(p))
									addNewCellToPatch(findPatch(p),x,y,hfloat);
								else {
#if SEASONAL
									pPatch = newPatch(seq++,p,nseasons);
#else
									pPatch = newPatch(seq++,p);
#endif // SEASONAL 
									addNewCellToPatch(pPatch,x,y,hfloat);
								}
							}
						}
						else { // cell-based model
							// add cell to landscape (patches created later)
							addNewCellToLand(x,y,hfloat);
						}
					}
				}
			}
			else { // additional habitat cover layers
				if (h != habnodata) {
					if (hfloat < 0.0 || hfloat > 100.0) { // invalid cover score
						hfile.close(); hfile.clear();
						if (patchModel) {
							pfile.close(); pfile.clear();
						}
						return 17;
					}
					else {
						cells[y][x]->setHabitat(hfloat);
					}
				} // end of h != habnodata
			}
#if RS_CONTAIN
			if (readdamage) {
				dfloat = baddfloat; dfile >> dfloat; d = (int)dfloat;
				if (d < 0) { // invalid damage value
					hfile.close(); hfile.clear();
					if (pfile.is_open()) { pfile.close(); pfile.clear(); }
					return 16;					
				}
				if (patchModel) {
					if (p == 0) // cell is in the matrix
						setDamage(0,x,y,d);
					else
						setDamage(pPatch,x,y,d);					
				}
				else setDamage(0,x,y,d);
			}
#endif // RS_CONTAIN 
		}
	}
	habIndexed = true; // habitats are already numbered 1...n in correct order
	break;

case 2: // habitat quality
	if (fileNum > 0) return 19; // error condition - should not occur
	for (int y = dimY-1; y >= 0; y--) {
		for (int x = 0; x < dimX; x++) {
			hfloat = badhfloat; hfile >> hfloat; h = (int)hfloat;
			if (patchModel) {
				pfloat = badpfloat; pfile >> pfloat; p = (int)pfloat;
			}
#if RSDEBUG
//MemoLine(("y=" + Int2Str(y) + " x=" + Int2Str(x) + " hfloat=" + Float2Str(hfloat)
//	+ " p=" + Int2Str(p)).c_str());
#endif
			if (h == habnodata) {
				addNewCellToLand(x,y,-1); // add cell only to landscape
			}
			else {
				if (hfloat < 0.0 || hfloat > 100.0) { // invalid quality score
					hfile.close(); hfile.clear();
					if (patchModel) {
						pfile.close(); pfile.clear();
					}
					return 17;
				}
				else {
					if (patchModel) {
						if (p < 0) { // invalid patch code
							hfile.close(); hfile.clear();
							pfile.close(); pfile.clear();
							return 14;
						}
						if (p == 0) { // cell is in the matrix
							addNewCellToPatch(0,x,y,hfloat);
						}
						else {
							if (existsPatch(p))
								addNewCellToPatch(findPatch(p),x,y,hfloat);
							else {
								addPatchNum(p);
#if SEASONAL
								pPatch = newPatch(seq++,p,nseasons);
#else
								pPatch = newPatch(seq++,p);
#endif // SEASONAL 
								addNewCellToPatch(pPatch,x,y,hfloat);
							}
						}
					}
					else { // cell-based model
						// add cell to landscape (patches created later)
						addNewCellToLand(x,y,hfloat);
					}
				}
			}
#if RS_CONTAIN
			if (readdamage) {
				dfloat = baddfloat; dfile >> dfloat; d = (int)dfloat;
				if (d < 0) { // invalid damage value
					hfile.close(); hfile.clear();
					if (pfile.is_open()) { pfile.close(); pfile.clear(); }
					return 16;					
				}
				if (patchModel) {
					if (p == 0) // cell is in the matrix
						setDamage(0,x,y,d);
					else
						setDamage(pPatch,x,y,d);					
				}
				else setDamage(0,x,y,d);
			}
#endif // RS_CONTAIN 
		}
	}
	break;

default:
	;
}

#if RS_CONTAIN
dmgLoaded = readdamage;
#endif // RS_CONTAIN 

if (hfile.is_open()) { hfile.close(); hfile.clear(); }
if (pfile.is_open()) { pfile.close(); pfile.clear(); }
#if RS_CONTAIN
if (dfile.is_open()) { dfile.close(); dfile.clear(); }
#endif // RS_CONTAIN 
return 0;
}

#if SPATIALMORT

//---------------------------------------------------------------------------

int Landscape::readMortalityFiles(string mortfile0,string mortfile1) {

string header;
int mortnodata[2];
int ncols,nrows;
float mfloat[2];
float badfloat[2];
bool errorvalue;
Cell *pCell;
//Patch *pPatch;
simParams sim = paramsSim->getSim();

ifstream mfile[2];

// open mortality files
mfile[0].open(mortfile0.c_str());
if (!mfile[0].is_open()) return 911;
mfile[1].open(mortfile1.c_str());
if (!mfile[1].is_open()) {
	mfile[0].close(); mfile[0].clear();
	return 912;
}
// read data from header records of mortality files
// NB headers of all files have already been compared
for (int i = 0; i < 2; i++) {
	mfile[i] >> header >> ncols >> header >> nrows >> header >> mfloat[i] >> header >> mfloat[i]
		>> header >> mfloat[i] >> header >> mortnodata[i];
	badfloat[i] = -9.0; if (mortnodata[i] == -9) badfloat[i] = -99.0;
}

for (int y = nrows-1; y >= 0; y--) {
	for (int x = 0; x < ncols; x++) {
		errorvalue = false;
		for (int i = 0; i < 2; i++) {
			mfloat[i] = badfloat[i]; mfile[i] >> mfloat[i];
			if (mfloat[i] == mortnodata[i]) // treat as zero mortality
				mfloat[i] = 0.0;
			if (mfloat[i] < 0.0 || mfloat[i] > 1.0) // invalid mortality rate
				errorvalue = true;
		}
#if DEBUG
//MemoLine(("y=" + Int2Str(y) + " x=" + Int2Str(x) + " hfloat=" + Float2Str(hfloat)
//	+ " p=" + Int2Str(p)).c_str());
#endif
		if (errorvalue) {
			for (int i = 0; i < 2; i++) {
				mfile[i].close(); mfile[i].clear();
			}
			return 917;
		}
		else {
			pCell = findCell(x,y);
			if (pCell != 0) { // not no-data cell
				pCell->setMort(mfloat[0],mfloat[1]);
			}
		}
	}
}

for (int i = 0; i < 2; i++) {
	if (mfile[i].is_open()) { mfile[i].close(); mfile[i].clear(); }
}
sim.mortMapLoaded = true;
paramsSim->setSim(sim);
return 0;
}

#endif

//---------------------------------------------------------------------------

int Landscape::readCosts(string fname)
{

ifstream costs;

//int hc,maxYcost,maxXcost,NODATACost,hab;
int hc,maxYcost,maxXcost,NODATACost;
float minLongCost, minLatCost; int resolCost;
float fcost;
string header;
Cell *pCell;
simView v = paramsSim->getViews();

int maxcost = 0;

#if RSDEBUG
#if BATCH
//DEBUGLOG << "Landscape::readCosts(): fname=" << fname << endl;
#endif
#endif
 // open cost file
costs.open(fname.c_str());
//if (!costs.is_open()) {
//	MessageDlg("COSTS IS NOT OPEN!!!!!",
//				mtError, TMsgDlgButtons() << mbRetry,0);
//}
//else {
//	MessageDlg("Costs is open!",
//				mtError, TMsgDlgButtons() << mbRetry,0);
//}
// read headers and check that they correspond to the landscape ones
costs >> header;
if (header != "ncols" && header != "NCOLS") {
//	MessageDlg("The selected file is not a raster.",
//	MessageDlg("Header problem in import_CostsLand()",
//				mtError, TMsgDlgButtons() << mbRetry,0);
	costs.close(); costs.clear();
	return -1;
}
costs >> maxXcost >> header >> maxYcost >> header >> minLongCost;
costs >> header >> minLatCost >> header >> resolCost >> header >> NODATACost;


MemoLine("Loading costs map. Please wait...");

for (int y = maxYcost - 1; y > -1; y--){
	for (int x = 0; x < maxXcost; x++){
		costs >> fcost; hc = (int)fcost; // read as float and convert to int
		if ((hc < 1 && hc != NODATACost)) {
#if RSDEBUG
#if BATCH
//		DEBUGLOG << "Landscape::readCosts(): x=" << x << " y=" << y 
//			<< " fcost=" << fcost << " hc=" << hc
//			<< endl;
#endif
#endif
			// error - zero / negative cost not allowed
//			MessageDlg("Error in the costs map file : zero or negative cost detected."
//			 , mtError, TMsgDlgButtons() << mbOK,0);
			costs.close(); costs.clear();
			return -999;
		}
		pCell = findCell(x,y);
		if (pCell != 0) { // not no-data cell
			pCell->setCost(hc);
			if (hc > maxcost) maxcost = hc;
		}
	}
}
costs.close(); costs.clear();

MemoLine("Costs map loaded.");

return maxcost;

}

//---------------------------------------------------------------------------

rasterdata CheckRasterFile(string fname)
{
rasterdata r;
string header;
int inint;
ifstream infile;

r.ok = true;
r.errors = r.ncols = r.nrows = r.cellsize = 0;
r.xllcorner = r.yllcorner = 0.0;

infile.open(fname.c_str());
if (infile.is_open()) {
	infile >> header >> r.ncols;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " r.ncols=" + Int2Str(r.ncols)
	 ).c_str());
#endif
	if (header != "ncols" && header != "NCOLS") r.errors++;
	infile >> header >> r.nrows;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " r.nrows=" + Int2Str(r.nrows)
	 ).c_str());
#endif
	if (header != "nrows" && header != "NROWS") r.errors++;
	infile >> header >> r.xllcorner;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " r.xllcorner=" + Float2Str(r.xllcorner)
	 ).c_str());
#endif
	if (header != "xllcorner" && header != "XLLCORNER") r.errors++;
	infile >> header >> r.yllcorner;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " r.yllcorner=" + Float2Str(r.yllcorner)
	 ).c_str());
#endif
	if (header != "yllcorner" && header != "YLLCORNER") r.errors++;
	infile >> header >> r.cellsize;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " r.cellsize=" + Int2Str(r.cellsize)
	 ).c_str());
#endif
	if (header != "cellsize" && header != "CELLSIZE") r.errors++;
	infile >> header >> inint;
#if RSDEBUG
DebugGUI(("CheckRasterFile(): header=" + header + " inint=" + Int2Str(inint)
	 ).c_str());
#endif
	if (header != "NODATA_value" && header != "NODATA_VALUE") r.errors++;
	infile.close();
	infile.clear();
	if (r.errors > 0) r.ok = false;
}
else {
	r.ok = false; r.errors = -111;
}
infile.clear();

return r;
}

//---------------------------------------------------------------------------

// Patch connectivity functions

// Create & initialise connectivity matrix
void Landscape::createConnectMatrix(void)
{
if (connectMatrix != 0) deleteConnectMatrix();
int npatches = (int)patches.size();
#if RSDEBUG
//DEBUGLOG << "Landscape::createConnectMatrix(): npatches=" << npatches << endl;
#endif
connectMatrix = new int *[npatches];
for (int i = 0; i < npatches; i++) {
	connectMatrix[i] = new int[npatches];
	for (int j = 0; j < npatches; j++) connectMatrix[i][j] = 0;
}
}

// Re-initialise connectivity matrix
void Landscape::resetConnectMatrix(void)
{
if (connectMatrix != 0) {
	int npatches = (int)patches.size();
	for (int i = 0; i < npatches; i++) {
		for (int j = 0; j < npatches; j++) connectMatrix[i][j] = 0;
	}
}
}

// Increment connectivity count between two specified patches
void Landscape::incrConnectMatrix(int p0,int p1) {
int npatches = (int)patches.size();
if (connectMatrix == 0 || p0 < 0 || p0 >= npatches || p1 < 0 || p1 >= npatches) return;
connectMatrix[p0][p1]++;
}

// Delete connectivity matrix
void Landscape::deleteConnectMatrix(void)
{
if (connectMatrix != 0) {
	int npatches = (int)patches.size();
	for (int j = 0; j < npatches; j++) {
		if (connectMatrix[j] != 0)
			delete connectMatrix[j];
	}
	delete[] connectMatrix;
	connectMatrix = 0;
}
}

// Write connectivity file headers
bool Landscape::outConnectHeaders(int option)
{
if (option == -999) { // close the file
	if (outConnMat.is_open()) outConnMat.close();
	outConnMat.clear();
	return true;
}

simParams sim = paramsSim->getSim();

string name = paramsSim->getDir(2);
if (sim.batchMode) {
	name += "Batch" + Int2Str(sim.batchNum) + "_";
	name += "Sim" + Int2Str(sim.simulation) + "_Land" + Int2Str(landNum);
}
else
	name += "Sim" + Int2Str(sim.simulation);
name += "_Connect.txt";
outConnMat.open(name.c_str());

#if SEASONAL
outConnMat << "Rep\tYear\tSeason\tStartPatch\tEndPatch\tNinds" << endl;
#else
outConnMat << "Rep\tYear\tStartPatch\tEndPatch\tNinds" << endl;
#endif // SEASONAL 

return outConnMat.is_open();
}

#if SEASONAL
void Landscape::outConnect(int rep,int yr,short season)
#else
void Landscape::outConnect(int rep,int yr)
#endif // SEASONAL 
{
int patchnum0,patchnum1;
int npatches = (int)patches.size();
int *emigrants  = new int[npatches]; // 1D array to hold emigrants from each patch
int *immigrants = new int[npatches]; // 1D array to hold immigrants to  each patch

for (int i = 0; i < npatches; i++) {
	emigrants[i] =  immigrants[i] = 0;
}

for (int i = 0; i < npatches; i++) {
	patchnum0 = patches[i]->getPatchNum();
	if (patchnum0 != 0) {
		for (int j = 0; j < npatches; j++) {
			patchnum1 = patches[j]->getPatchNum();
			if (patchnum1 != 0) {
				emigrants[i]  += connectMatrix[i][j];
				immigrants[j] += connectMatrix[i][j];
				if (connectMatrix[i][j] > 0) {
					outConnMat << rep << "\t" << yr 
#if SEASONAL
						<< "\t" << season 
#endif  
						<< "\t" << patchnum0 << "\t" << patchnum1 
						<< "\t" << connectMatrix[i][j] << endl;
				}
			}
		}
	}
}

for (int i = 0; i < npatches; i++) {
	patchnum0 = patches[i]->getPatchNum();
	if (patchnum0 != 0) {
#if SEASONAL
		if (patches[i]->getK(season) > 0.0) 
#else
		if (patches[i]->getK() > 0.0) 
#endif // SEASONAL 
		{ // suitable patch
			outConnMat << rep << "\t" << yr 
#if SEASONAL
						<< "\t" << season 
#endif  
				<< "\t" << patchnum0 << "\t-999\t" << emigrants[i] << endl;
			outConnMat << rep << "\t" << yr 
#if SEASONAL
						<< "\t" << season 
#endif  
				<< "\t-999\t" << patchnum0 << "\t" << immigrants[i] << endl;
		}
	}
}

delete[] emigrants;
delete[] immigrants;

}

#if RS_ABC
// Returns connectivity (no. of successful dispersers) for given start and end patches
int Landscape::outABCconnect(int startpatch,int endpatch)
{
int npred,patch0,patch1;
npred = -666; patch0 = patch1 = -1;
int npatches = (int)patches.size();
for (int i = 0; i < npatches; i++) {
	if (startpatch == patches[i]->getPatchNum()) patch0 = i;
	if (endpatch   == patches[i]->getPatchNum()) patch1 = i;
}
#if RSDEBUG
DEBUGLOG << "Landscape::outABCconnect(): npatches=" << npatches
	<< " startpatch=" << startpatch << " patch0=" << patch0
	<< " endpatch=" << endpatch << " patch1=" << patch1
	<< endl;
#endif
if (startpatch == -999 || endpatch == -999) {
	// calculate appropriate marginal total of immigrants / emigrants
	int total = 0;
	if (startpatch == -999 && patch1 >= 0) {
		for (int i = 0; i < npatches; i++) {
			total += connectMatrix[i][patch1];
		}
		npred = total;
	}
	else {
		if (endpatch == -999 && patch0 >= 0) {
			for (int i = 0; i < npatches; i++) {
				total += connectMatrix[patch0][i];
			}
			npred = total;
		}
	}
}
else {
	if (patch0 >= 0 && patch1 >= 0) {
		npred = connectMatrix[patch0][patch1];
	}
}
#if RSDEBUG
DEBUGLOG << "Landscape::outABCconnect(): npred=" << npred
	<< endl;
#endif

return npred;
}
#endif

//---------------------------------------------------------------------------

#if HEATMAP

void Landscape::resetVisits(void) {
for(int y = dimY-1; y >= 0; y--){
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] != 0) { // not a no-data cell
			cells[y][x]->resetVisits();
		}
	}
}
}

// Save SMS path visits map to raster text file
void Landscape::outVisits(int rep, int landNr) {

string name;
simParams sim = paramsSim->getSim();

if (sim.batchMode) {
	name = paramsSim->getDir(3)
		+ "Batch" + Int2Str(sim.batchNum) + "_"
		+ "Sim" + Int2Str(sim.simulation)
		+ "_land" + Int2Str(landNr) + "_rep" + Int2Str(rep)
//		+ "_yr" + Int2Str(yr)
		+ "_Visits.txt";
}
else {
	name = paramsSim->getDir(3)
		+ "Sim" + Int2Str(sim.simulation)
		+ "_land" + Int2Str(landNr) + "_rep" + Int2Str(rep)
//		+ "_yr" + Int2Str(yr)
		+ "_Visits.txt";
}
outvisits.open(name.c_str());

outvisits << "ncols " << dimX << endl;
outvisits << "nrows " << dimY << endl;
outvisits << "xllcorner " << minEast << endl;
outvisits << "yllcorner " << minNorth << endl;
outvisits << "cellsize " << resol << endl;
outvisits << "NODATA_value -9" << endl;

for (int y = dimY-1; y >= 0; y--) {
#if RSDEBUG
//DebugGUI(("Landscape::drawLandscape(): y=" + Int2Str(y)
//	+ " cells[y]=" + Int2Str((int)cells[y])).c_str());
#endif
	for (int x = 0; x < dimX; x++) {
		if (cells[y][x] == 0) { // no-data cell
			outvisits << "-9 ";
		}
		else {
			outvisits << cells[y][x]->getVisits() << " ";
		}
	}
	outvisits << endl;
}

outvisits.close(); outvisits.clear();
}
#endif

//---------------------------------------------------------------------------

#if SEASONAL
//#if PARTMIGRN

// extreme events

void Landscape::addExtEvent(extEvent e) { 
#if RSDEBUG
//DebugGUI(("Landscape::addExtEvent(): e.year=" + Int2Str(e.year)
//	+ " e.season=" + Int2Str(e.season)
//	+ " e.patchID=" + Int2Str(e.patchID)
//	+ " e.x=" + Int2Str(e.x) + " e.y=" + Int2Str(e.y)
//	).c_str());
#endif
extevents.push_back(e);
}

extEvent Landscape::getExtEvent(int ix) { 
extEvent e;
if (ix >= 0 && ix < (int)extevents.size()) {
	e = extevents[ix];
}
else {
	e.year = e.season = e.patchID = e.x = e.y = 0;
	e.probMort = 0.0;
}
#if RSDEBUG
//DEBUGLOG << "Landscape::getExtEvent(): ix=" << ix << " size()=" << extevents.size()
//	<< " e.year=" << e.year << " e.season=" << e.season
//	<< " e.patchID=" << e.patchID << " e.x=" << e.x << " e.y=" << e.y
//	<< endl;
#endif
return e;
}

void Landscape::resetExtEvents(void) { extevents.clear(); }

int Landscape::numExtEvents(void) { return (int)extevents.size(); }

//#endif // PARTMIGRN 
#endif // SEASONAL 

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


