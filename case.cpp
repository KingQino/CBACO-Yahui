#include "case.h"

#include <stdexcept>

Case::Case(string filename, int ID) {
	this->ID = ID;
	this->filename = filename;
	this->depotNumber = 1;
	this->depot = 0;
	this->customerNumber = 0;
	this->stationNumber = 0;
	this->vehicleNumber = 0;
	this->maxC = 0;
	this->maxQ = 0;
	this->conR = 0;

	ifstream infile(filename.c_str());
	if (!infile.is_open()) {
		throw runtime_error("Failed to open instance file: " + filename);
	}

	auto trim = [](const string& text) {
		const size_t first = text.find_first_not_of(" \t\r\n");
		if (first == string::npos) {
			return string();
		}
		const size_t last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	};

	auto valueAfterColon = [](const string& text) {
		const size_t colon = text.find(':');
		if (colon == string::npos) {
			return string();
		}
		return text.substr(colon + 1);
	};

	vector<pair<int, pair<double, double>>> coordinateEntries;
	vector<pair<int, double>> demandEntries;
	int declaredDimension = 0;
	int declaredStations = 0;
	int maxCoordinateIndex = 0;
	int maxDemandIndex = 0;
	bool readingCoordinates = false;
	bool readingDemand = false;

	char line[250];
	while (infile.getline(line, 249)) {
		string templine = trim(line);
		if (templine.empty()) {
			continue;
		}

		if (readingCoordinates) {
			if (templine.find("DEMAND_SECTION") != string::npos) {
				readingCoordinates = false;
				readingDemand = true;
				continue;
			}

			stringstream lineStream(templine);
			int ind;
			double x;
			double y;
			if (!(lineStream >> ind >> x >> y)) {
				throw runtime_error("Malformed coordinate line in instance file: " + filename);
			}

			coordinateEntries.push_back(make_pair(ind, make_pair(x, y)));
			maxCoordinateIndex = max(maxCoordinateIndex, ind);
			continue;
		}

		if (readingDemand) {
			if (templine.find("STATIONS_COORD_SECTION") != string::npos ||
				templine.find("DEPOT_SECTION") != string::npos ||
				templine == "EOF") {
				readingDemand = false;
				continue;
			}

			stringstream lineStream(templine);
			int ind;
			double c;
			if (!(lineStream >> ind >> c)) {
				throw runtime_error("Malformed demand line in instance file: " + filename);
			}

			demandEntries.push_back(make_pair(ind, c));
			maxDemandIndex = max(maxDemandIndex, ind);
			continue;
		}

		if (templine.find("DIMENSION:") != string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> declaredDimension;
		}
		else if (templine.find("STATIONS:") != string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> declaredStations;
		}
		else if (templine.find("VEHICLES:") != string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> this->vehicleNumber;
		}
		else if (templine.find("CAPACITY:") != string::npos && templine.find("ENERGY") == string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> this->maxC;
		}
		else if (templine.find("ENERGY_CAPACITY:") != string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> this->maxQ;
		}
		else if (templine.find("ENERGY_CONSUMPTION:") != string::npos) {
			stringstream valueStream(valueAfterColon(templine));
			valueStream >> this->conR;
		}
		else if (templine.find("NODE_COORD_SECTION") != string::npos) {
			readingCoordinates = true;
		}
		else if (templine.find("DEMAND_SECTION") != string::npos) {
			readingDemand = true;
		}
	}

	if (coordinateEntries.empty() || demandEntries.empty()) {
		throw runtime_error("Missing coordinate or demand section in instance file: " + filename);
	}

	const int parsedTotalNumber = maxCoordinateIndex;
	const int depotAndCustomerCount = maxDemandIndex;
	this->customerNumber = depotAndCustomerCount - this->depotNumber;
	this->stationNumber = parsedTotalNumber - depotAndCustomerCount;

	if (this->customerNumber < 0 || this->stationNumber < 0) {
		throw runtime_error("Inconsistent node counts in instance file: " + filename);
	}
	if (declaredStations > 0 && declaredStations != this->stationNumber) {
		throw runtime_error("Station count mismatch in instance file: " + filename);
	}
	if (declaredDimension > 0 &&
		declaredDimension != depotAndCustomerCount &&
		declaredDimension != parsedTotalNumber) {
		throw runtime_error("Unsupported DIMENSION definition in instance file: " + filename);
	}

	this->positions.assign(parsedTotalNumber, make_pair(0.0, 0.0));
	for (const auto& entry : coordinateEntries) {
		this->positions[entry.first - 1] = entry.second;
	}

	this->demand.assign(depotAndCustomerCount, 0);
	for (const auto& entry : demandEntries) {
		this->demand[entry.first - 1] = entry.second;
		if (entry.second == 0) {
			this->depot = entry.first - 1;
		}
	}

	if (this->conR <= 0) {
		throw runtime_error("Invalid energy consumption in instance file: " + filename);
	}

	int totalNumber = depotNumber + customerNumber + stationNumber;
	this->distances = new double* [totalNumber];
	for (int i = 0; i < totalNumber; i++) {
		this->distances[i] = new double[totalNumber];
	}
	for (int i = 0; i < totalNumber; i++) {
		for (int j = i; j < totalNumber; j++) {
			if (i == j) {
				this->distances[i][j] = 0;
			}
			else {
				double xx = this->positions[i].first - this->positions[j].first;
				double yy = this->positions[i].second - this->positions[j].second;
				this->distances[i][j] = this->distances[j][i] = sqrt(xx * xx + yy * yy);
			}
		}
	}
	this->maxDis = maxQ / conR;
	this->posflag = false;

	this->bestStations = new vector<int>* [depotNumber + customerNumber];
	for (int i = 0; i < depotNumber + customerNumber; i++) {
		this->bestStations[i] = new vector<int>[depotNumber + customerNumber];
		//memset(this->bestStations[i], 0, sizeof(int) * (depotNumber + customerNumber));
	}
	for (int i = 0; i < depotNumber + customerNumber - 1; i++) {
		for (int j = i + 1; j < depotNumber + customerNumber; j++) {
			this->bestStations[i][j] = this->bestStations[j][i] = findTheNonDominatedStations(i, j);
		}
	}

	this->bestStation = new int* [depotNumber + customerNumber];
	for (int i = 0; i < depotNumber + customerNumber; i++) {
		this->bestStation[i] = new int[depotNumber + customerNumber];
		memset(this->bestStation[i], 0, sizeof(int)* (depotNumber + customerNumber));
	}
	for (int i = 0; i < depotNumber + customerNumber - 1; i++) {
		for (int j = i + 1; j < depotNumber + customerNumber; j++) {
			this->bestStation[i][j] = this->bestStation[j][i] = findNearestStation(i, j);
		}
	}

	this->totalDem = 0;
	for (auto& e : demand) {
		this->totalDem += e;
	}

	this->candidatelist = getCandiList2(20);

	// customized variables by Yinghao
	this->actualProblemSize = this->depotNumber + this->customerNumber + this->stationNumber;
	this->evals = 0.0;
	this->maxEvals = this->actualProblemSize * MAX_EVALUAION_FACTOR;
}

vector<vector<int>> Case::getCandiList2(int candino) {
	vector<set<int>> candilist = getCandiList(candino);
	vector<vector<int>> candilist2;
	for (auto a : candilist) {
		vector<int> temp;
		for (auto b : a) {
			temp.push_back(b);
		}
		candilist2.push_back(temp);
	}
	return candilist2;
}

vector<set<int>> Case::getCandiList(int candino) {
	vector<set<int>> candilist;
	for (int i = 0; i < this->depotNumber + this->customerNumber; i++) {
		set<int> onelist;
		int largeone = -1;
		double largedis = 0;
		for (int j = 0; j < this->depotNumber + this->customerNumber; j++) {
			if (i == j) continue;
			if ((int)onelist.size() < candino) {
				onelist.insert(j);
				if (largedis < this->distances[i][j]) {
					largedis = this->distances[i][j];
					largeone = j;
				}
			}
			else {
				if (largedis > this->distances[i][j]) {
					onelist.erase(largeone);
					onelist.insert(j);
					largedis = 0;
					for (auto e : onelist) {
						if (this->distances[i][e] > largedis) {
							largedis = this->distances[i][e];
							largeone = e;
						}
					}
				}
			}
		}
		candilist.push_back(onelist);
	}
	return candilist;
}

vector<int> Case::findTheNonDominatedStations(int x, int y) {
	vector<int> temp;
	for (int i = this->customerNumber + this->depotNumber; i < this->customerNumber + this->depotNumber + this->stationNumber; i++) {
		bool bedominated = false;
		for (auto e : temp) {
			if (this->distances[x][e] <= this->distances[x][i] &&
				this->distances[e][y] <= this->distances[i][y]) {
				bedominated = true;
				break;
			}
		}
		if (bedominated == false) {
			for (int j = 0; j < (int)temp.size(); j++) {
				if (this->distances[x][i] <= this->distances[x][temp[j]] &&
					this->distances[i][y] <= this->distances[temp[j]][y]) {
					temp.erase(temp.begin() + j);
					j--;
				}
			}
			temp.push_back(i);
		}
	}
	return temp;
}

Case::~Case() {
	int totalNumber = depotNumber + customerNumber + stationNumber;
	for (int i = 0; i < totalNumber; i++) {
		delete[] this->distances[i];
	}
	delete[] this->distances;
	for (int i = 0; i < depotNumber + customerNumber; i++) {
		delete[] this->bestStation[i];
		delete[] this->bestStations[i];
	}
	delete[] this->bestStation;
	delete[] this->bestStations;
}

double Case::getDistance(int i, int j) {
	this->evals += (1.0 / this->actualProblemSize);

	return this->distances[i][j];
}

double Case::getEnergyDemand(int i, int j) {
	return this->distances[i][j] * this->conR;
}

double Case::calculateRouteDistance(vector<int> x) {
	double sum = 0;
	double piecesum = 0;
	double piececap = 0;
	for (int i = 0; i < (int)x.size() - 1; i++) {
		piecesum += distances[x[i]][x[i + 1]];
		if (x[i] == 0) piececap = 0;
		else if (x[i] > 0 && x[i] < depotNumber + customerNumber) piececap += demand[x[i]];
		sum += distances[x[i]][x[i + 1]];
		if (piecesum > maxDis || piececap > maxC) return -1 * piecesum;
		if (x[i + 1] < depotNumber || x[i + 1] >= depotNumber + customerNumber)
			piecesum = 0;
	}
	return sum;
}

double Case::calculateRouteDistance(int* x, int length) {
	double sum = 0;
	double piecesum = 0;
	for (int i = 0; i < length - 1; i++) {
		piecesum += distances[x[i]][x[i + 1]];
		sum += distances[x[i]][x[i + 1]];
		if (piecesum > maxDis) return -1 * piecesum;
		if (x[i + 1] < depotNumber || x[i + 1] >= depotNumber + customerNumber)
			piecesum = 0;
	}
	return sum;
}

double Case::calculateRouteDistance(int* x) {
	double sum = 0;
	int totalNumber = depotNumber + customerNumber + stationNumber;
	int counter = 0;
	while (!(x[counter + 1] >= totalNumber || x[counter + 1] < 0))
	{
		sum += distances[x[counter]][x[counter + 1]];
	}
	return sum;
}

int Case::findNearestStation(int x) {
	int thestation = -1;
	double bigdis = DBL_MAX;
	for (int i = depotNumber + customerNumber; i < depotNumber + customerNumber + stationNumber; i++) {
		if (bigdis > distances[x][i] && x != i) {
			thestation = i;
			bigdis = distances[x][i];
		}
	}
	return thestation;
}

int Case::findNearestStation(int x, int y) {
	int thestation = -1;
	double bigdis = DBL_MAX;
	for (int i = depotNumber + customerNumber; i < depotNumber + customerNumber + stationNumber; i++) {
		if (bigdis > distances[x][i] + distances[y][i] && x != i && y != i) {
			thestation = i;
			bigdis = distances[x][i] + distances[y][i];
		}
	}
	return thestation;
}

//reture the station within maxdis from x, and min dis[x][s]+dis[y][s]
int Case::findNearestStationFeasible(int x, int y, double maxdis) {
	int thestation = -1;
	double bigdis = DBL_MAX;
	for (int i = depotNumber + customerNumber; i < depotNumber + customerNumber + stationNumber; i++) {
		if (distances[x][i] < maxdis && bigdis > distances[x][i] + distances[y][i] && i != x && y != i && distances[i][y] < maxDis) {
			thestation = i;
			bigdis = distances[x][i] + distances[y][i];
		}
	}
	return thestation;
}

int Case::findNearestStationFeasible2(int x, int y, double maxdis) {
	int thestation = -1;
	double bigdis = DBL_MAX;
	for (int i = depotNumber + customerNumber; i < depotNumber + customerNumber + stationNumber; i++) {
		if (distances[x][i] < maxdis && bigdis > distances[x][i] + distances[y][i] && i != x && y != i && distances[i][y] < maxDis) {
			thestation = i;
			bigdis = distances[x][i] + distances[y][i];
		}
	}
	if (distances[x][0] < maxdis && bigdis > distances[x][0] + distances[y][0] && 0 != x && y != 0 && distances[0][y] < maxDis) {
		thestation = 0;
		bigdis = distances[x][0] + distances[y][0];
	}
	return thestation;
}

void Case::writeAllPositions() {
	string outfilename = filename.substr(0, filename.find(".evrp") + 1) + "pos";
	ofstream oufile(outfilename.c_str());
	oufile << depotNumber + customerNumber << endl;
	for (auto a : positions) {
		oufile << a.first << ',' << a.second << endl;
	}
	oufile.close();
	posflag = true;
}

void Case::drawARoute(vector<int> route, string picname) {
	if (posflag == false) {
		writeAllPositions();
	}
	string solufilename = "tempsolution.txt";
	string posfilename = filename.substr(0, filename.find(".evrp") + 1) + "pos";
	ofstream solufile(solufilename.c_str());
	for (int i = 0; i < (int)route.size(); i++) {
		solufile << route[i];
		if (i != (int)route.size() - 1) {
			solufile << ',';
		}
	}
	solufile << endl;
	string command = "python ./draw.py " + posfilename + " " + picname;
	system(command.c_str());
	remove(solufilename.c_str());
}

void Case::testTheStationReach() {
	bool flag = false;
	for (int i = 0; i < stationNumber; i++) {
		if (maxDis < distances[0][1 + customerNumber + i]) {
			cout << 1 + customerNumber + i << endl;
			flag = true;
		}
	}
	if (flag) {
		cout << "There is station that cannot be directly reached " << ID << endl;
	}
	else {
		cout << "all good" << endl;
	}
}

void Case::checkASoluton(string filename) {
	ifstream ifile(filename.c_str());
	vector<int> arr;
	string solustr;
	getline(ifile, solustr);
	getline(ifile, solustr);
	istringstream is(solustr);
	int node;
	char dot;
	vector<int> arr1;
	while (is >> node)
	{
		arr1.push_back(node);
		is >> dot;
	}
	vector<vector<int>> solution;
	for (int i = 0; i < (int)arr1.size(); i++) {
		if (arr1[i] == 0) {
			vector<int> temp;
			solution.push_back(temp);
		}
		solution[solution.size() - 1].push_back(arr1[i]);
	}
	for (int i = 0; i < (int)solution.size(); i++) {
		solution[i].push_back(0);
	}
	//bool capcheck = true;
	//bool elecheck = true;
	for (int i = 0; i < (int)solution.size(); i++) {
		double totc = 0;
		double totd = 0;
		for (int j = 1; j < (int)solution[i].size(); j++) {
			if (solution[i][j - 1] == 0 || solution[i][j - 1] >= 1 + customerNumber) {
				totd = distances[solution[i][j - 1]][solution[i][j]];
			}
			else {
				totd += distances[solution[i][j - 1]][solution[i][j]];
			}
			if (solution[i][j] > 0 && solution[i][j] < 1 + customerNumber) {
				totc += demand[solution[i][j]];
			}
			if (totc > maxC || totd > maxDis) {
				cout << filename << endl;
				return;
			}
		}
	}
}

double Case::getEvals() const {
	return this->evals;
}
