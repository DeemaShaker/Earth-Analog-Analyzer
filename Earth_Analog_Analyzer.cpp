/*
 * Copyright 2026 Deema Shaker
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int CRITERIA_COUNT = 3;
const int MIN_CRITERIA_TO_RANK = 1;

const array<string, CRITERIA_COUNT> criteriaNames = {
    "Surface temperature (C)",
    "Terrain slope (degrees)",
    "Local relief (meters)"
};

const array<double, CRITERIA_COUNT> allowedMin = {
    -273.15, 0.0, 0.0
};

const array<double, CRITERIA_COUNT> allowedMax = {
    1000.0, 90.0, 100000.0
};

enum class RangeStatus {
    VALID,
    MISSING,
    INVALID
};

struct Range {
    double minValue = 0.0;
    double maxValue = 0.0;
    RangeStatus status = RangeStatus::MISSING;
};

struct Profile {
    string name;
    string target;
    string source;
    array<Range, CRITERIA_COUNT> ranges;
};

struct CriterionResult {
    bool comparable = false;
    double iou = 0.0;
};

struct Result {
    string siteName;
    array<CriterionResult, CRITERIA_COUNT> criteria;
    int comparableCount = 0;
    double averageIoU = 0.0;
};

// Remove spaces from the start and end of a string.
string trim(const string& value) {
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";

    size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

// Split a line using semicolons. Empty columns are kept.
vector<string> splitLine(const string& line) {
    vector<string> parts;
    string part;
    stringstream input(line);

    while (getline(input, part, ';')) {
        parts.push_back(trim(part));
    }

    if (!line.empty() && line.back() == ';') {
        parts.push_back("");
    }

    return parts;
}

// Check whether a value means that the measurement is not available.
bool isMissing(const string& value) {
    string text = value;
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });

    return text.empty() || text == "?" || text == "na" || text == "n/a";
}

// Read a number only if the whole field contains a valid number.
bool readNumber(const string& text, double& number) {
    try {
        size_t used = 0;
        number = stod(text, &used);
        return used == text.size() && isfinite(number);
    }
    catch (...) {
        return false;
    }
}

// Read one minimum/maximum pair and check that it makes sense.
Range readRange(const string& minText, const string& maxText, int criterion) {
    Range range;

    bool minMissing = isMissing(minText);
    bool maxMissing = isMissing(maxText);

    if (minMissing && maxMissing) {
        range.status = RangeStatus::MISSING;
        return range;
    }

    // A pair must be complete. One value alone is not enough.
    if (minMissing || maxMissing) {
        range.status = RangeStatus::INVALID;
        return range;
    }

    double minValue, maxValue;
    if (!readNumber(minText, minValue) || !readNumber(maxText, maxValue)) {
        range.status = RangeStatus::INVALID;
        return range;
    }

    if (minValue >= maxValue ||
        minValue < allowedMin[criterion] ||
        maxValue > allowedMax[criterion]) {
        range.status = RangeStatus::INVALID;
        return range;
    }

    range.minValue = minValue;
    range.maxValue = maxValue;
    range.status = RangeStatus::VALID;
    return range;
}

// Read the Earth file or the Mars/Moon reference file.
vector<Profile> readProfiles(const string& fileName, bool isReference) {
    vector<Profile> profiles;
    ifstream file(fileName);

    if (!file.is_open()) {
        cerr << "Could not open file: " << fileName << "\n";
        return profiles;
    }

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);

        if (line.empty() || line[0] == '#') continue;

        // Remove the UTF-8 marker if it appears at the start of the file.
        if (lineNumber == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
        }

        vector<string> fields = splitLine(line);
        int expectedColumns = isReference ? 9 : 8;

        if (!fields.empty() &&
            ((isReference && fields[0] == "target") ||
             (!isReference && fields[0] == "name"))) {
            continue;
        }

        if (static_cast<int>(fields.size()) != expectedColumns) {
            cerr << "Line " << lineNumber << " in " << fileName
                 << " was skipped: expected " << expectedColumns
                 << " columns, found " << fields.size() << ".\n";
            continue;
        }

        Profile profile;
        int firstRangeColumn;

        if (isReference) {
            profile.target = fields[0];
            profile.name = fields[1];
            profile.source = fields[2];
            firstRangeColumn = 3;
        }
        else {
            profile.name = fields[0];
            profile.source = fields[1];
            firstRangeColumn = 2;
        }

        if (profile.name.empty()) {
            cerr << "Line " << lineNumber << " in " << fileName
                 << " was skipped: the name is empty.\n";
            continue;
        }

        bool hasInvalidRange = false;

        for (int i = 0; i < CRITERIA_COUNT; i++) {
            profile.ranges[i] = readRange(
                fields[firstRangeColumn + i * 2],
                fields[firstRangeColumn + i * 2 + 1], i
            );

            if (profile.ranges[i].status == RangeStatus::INVALID) {
                hasInvalidRange = true;
                cerr << "Warning: " << profile.name << " has an invalid range for "
                     << criteriaNames[i] << ". It will be skipped.\n";
            }
        }

        if (hasInvalidRange) {
            cerr << "Check the values for " << profile.name << " in "
                 << fileName << ".\n";
        }

        profiles.push_back(profile);
    }

    return profiles;
}

// Find the part shared by two ranges.
double overlapLength(const Range& first, const Range& second) {
    double low = max(first.minValue, second.minValue);
    double high = min(first.maxValue, second.maxValue);
    return max(0.0, high - low);
}

// IoU is the shared part divided by the total span covered by both ranges.
// It treats the two ranges equally, unlike one-sided coverage.
double calculateIoU(const Range& first, const Range& second) {
    double overlap = overlapLength(first, second);
    double firstWidth = first.maxValue - first.minValue;
    double secondWidth = second.maxValue - second.minValue;
    double unionLength = firstWidth + secondWidth - overlap;

    if (unionLength <= 0.0) return 0.0;
    return overlap / unionLength * 100.0;
}

// Compare one Earth location with one reference region.
Result analyzeSite(const Profile& site, const Profile& reference) {
    Result result;
    result.siteName = site.name;
    double total = 0.0;

    for (int i = 0; i < CRITERIA_COUNT; i++) {
        const Range& earthRange = site.ranges[i];
        const Range& referenceRange = reference.ranges[i];

        // Missing values are left out. They are not treated as zero.
        if (earthRange.status != RangeStatus::VALID ||
            referenceRange.status != RangeStatus::VALID) {
            continue;
        }

        result.criteria[i].comparable = true;
        result.criteria[i].iou = calculateIoU(earthRange, referenceRange);
        total += result.criteria[i].iou;
        result.comparableCount++;
    }

    if (result.comparableCount > 0) {
        result.averageIoU = total / result.comparableCount;
    }

    return result;
}

// Show the measurements that could and could not be compared.
void printResult(const Profile& site, const Profile& reference, const Result& result) {
    cout << "\n" << site.name << "\n";
    cout << "----------------------------------------\n";

    for (int i = 0; i < CRITERIA_COUNT; i++) {
        cout << criteriaNames[i] << ": ";

        if (!result.criteria[i].comparable) {
            bool invalid = site.ranges[i].status == RangeStatus::INVALID
                || reference.ranges[i].status == RangeStatus::INVALID;

            if (invalid) {
                cout << "Not compared (the range is invalid).\n";
            }
            else {
                cout << "Not compared (a measurement is missing).\n";
            }
            continue;
        }

        cout << fixed << setprecision(1)
             << "IoU overlap = " << result.criteria[i].iou << "%\n";
    }

    cout << "Comparable criteria: " << result.comparableCount
         << " / " << CRITERIA_COUNT << "\n";

    if (result.comparableCount > 0) {
        cout << "Average IoU: " << fixed << setprecision(1)
             << result.averageIoU << "%\n";
    }
    else {
        cout << "No score: there are no measurements to compare.\n";
    }
}

// Higher average IoU comes first. If scores are equal, use more data first.
bool betterResult(const Result& first, const Result& second) {
    if (first.averageIoU != second.averageIoU) {
        return first.averageIoU > second.averageIoU;
    }

    if (first.comparableCount != second.comparableCount) {
        return first.comparableCount > second.comparableCount;
    }

    return first.siteName < second.siteName;
}

void printRanking(vector<Result> results) {
    vector<Result> ranked;

    for (const Result& result : results) {
        if (result.comparableCount >= MIN_CRITERIA_TO_RANK) {
            ranked.push_back(result);
        }
    }

    sort(ranked.begin(), ranked.end(), betterResult);

    cout << "\nPRELIMINARY ORDER\n";
    cout << "----------------------------------------\n";

    if (ranked.empty()) {
        cout << "There is not enough data to compare these locations.\n";
        return;
    }

    for (size_t i = 0; i < ranked.size(); i++) {
        cout << i + 1 << ". " << ranked[i].siteName
             << " | Average IoU: " << fixed << setprecision(1)
             << ranked[i].averageIoU << "%"
             << " | Criteria used: " << ranked[i].comparableCount
             << "/" << CRITERIA_COUNT << "\n";
    }

    cout << "\nThis order uses only the measurements that were available on both sides.\n";
    cout << "A result based on one criterion is very limited and should not be\n";
    cout << "read as proof that the location is a good planetary analog.\n";
}

// A few checks for the main calculation and the ranking order.
bool runTests() {
    int passed = 0;
    int failed = 0;

    auto check = [&](const string& name, double actual, double expected) {
        if (fabs(actual - expected) < 0.000001) {
            cout << "PASS: " << name << "\n";
            passed++;
        }
        else {
            cout << "FAIL: " << name << " (got " << actual
                 << ", expected " << expected << ")\n";
            failed++;
        }
    };

    Range a{0.0, 10.0, RangeStatus::VALID};
    Range b{5.0, 15.0, RangeStatus::VALID};
    Range c{0.0, 20.0, RangeStatus::VALID};
    Range d{20.0, 30.0, RangeStatus::VALID};

    check("Same range", calculateIoU(a, a), 100.0);
    check("Half overlap", calculateIoU(a, b), 33.3333333333);
    check("One range inside another", calculateIoU(a, c), 50.0);
    check("No overlap", calculateIoU(a, d), 0.0);

    Range touching{10.0, 20.0, RangeStatus::VALID};
    check("Ranges that only touch", calculateIoU(a, touching), 0.0);

    if (readRange("?", "?", 0).status == RangeStatus::MISSING) {
        cout << "PASS: Missing pair is kept as missing\n";
        passed++;
    }
    else {
        cout << "FAIL: Missing pair is kept as missing\n";
        failed++;
    }

    if (readRange("5", "?", 0).status == RangeStatus::INVALID) {
        cout << "PASS: Half-filled pair is rejected\n";
        passed++;
    }
    else {
        cout << "FAIL: Half-filled pair is rejected\n";
        failed++;
    }

    Result first;
    first.siteName = "First";
    first.averageIoU = 40.0;
    first.comparableCount = 1;

    Result second;
    second.siteName = "Second";
    second.averageIoU = 60.0;
    second.comparableCount = 1;

    if (betterResult(second, first)) {
        cout << "PASS: Higher IoU comes first\n";
        passed++;
    }
    else {
        cout << "FAIL: Higher IoU comes first\n";
        failed++;
    }

    if (readRange("-300", "0", 0).status == RangeStatus::INVALID) {
        cout << "PASS: Temperature below absolute zero is rejected\n";
        passed++;
    }
    else {
        cout << "FAIL: Temperature below absolute zero is rejected\n";
        failed++;
    }

    if (readRange("10", "5", 0).status == RangeStatus::INVALID) {
        cout << "PASS: Reversed range is rejected\n";
        passed++;
    }
    else {
        cout << "FAIL: Reversed range is rejected\n";
        failed++;
    }

    if (readRange("5", "5", 1).status == RangeStatus::INVALID) {
        cout << "PASS: Equal minimum and maximum are rejected\n";
        passed++;
    }
    else {
        cout << "FAIL: Equal minimum and maximum are rejected\n";
        failed++;
    }

    Profile sampleSite;
    sampleSite.name = "Sample";
    sampleSite.ranges[0] = Range{0.0, 10.0, RangeStatus::VALID};

    Profile sampleReference;
    sampleReference.ranges[0] = Range{0.0, 10.0, RangeStatus::VALID};
    sampleReference.ranges[1] = Range{0.0, 10.0, RangeStatus::VALID};

    Result missingSlope = analyzeSite(sampleSite, sampleReference);
    check("Missing slope is left out of the average", missingSlope.averageIoU, 100.0);

    if (missingSlope.comparableCount == 1 && !missingSlope.criteria[1].comparable) {
        cout << "PASS: Missing slope is not counted\n";
        passed++;
    }
    else {
        cout << "FAIL: Missing slope is not counted\n";
        failed++;
    }

    Profile separated;
    separated.ranges[0] = Range{20.0, 30.0, RangeStatus::VALID};
    Result noOverlap = analyzeSite(sampleSite, separated);
    check("No overlap is scored as zero", noOverlap.averageIoU, 0.0);

    if (noOverlap.comparableCount == 1) {
        cout << "PASS: No overlap still counts as one criterion\n";
        passed++;
    }
    else {
        cout << "FAIL: No overlap still counts as one criterion\n";
        failed++;
    }

    sampleSite.ranges[0] = readRange("20", "10", 0);
    sampleSite.ranges[1] = Range{0.0, 10.0, RangeStatus::VALID};
    Result invalidTemperature = analyzeSite(sampleSite, sampleReference);
    check("A valid slope still counts when temperature is invalid",
          invalidTemperature.averageIoU, 100.0);

    if (invalidTemperature.comparableCount == 1 &&
        !invalidTemperature.criteria[0].comparable) {
        cout << "PASS: Invalid temperature is not scored as zero overlap\n";
        passed++;
    }
    else {
        cout << "FAIL: Invalid temperature is not scored as zero overlap\n";
        failed++;
    }

    cout << "\nTests passed: " << passed << " | Failed: " << failed << "\n";
    return failed == 0;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && string(argv[1]) == "--test") {
        return runTests() ? 0 : 1;
    }

    string earthFile = "earth_data.txt";
    string referenceFile = "reference_data.txt";

    if (argc == 3) {
        earthFile = argv[1];
        referenceFile = argv[2];
    }
    else if (argc != 1) {
        cerr << "Usage: " << argv[0]
             << " [earth_data.txt reference_data.txt | --test]\n";
        return 1;
    }

    cout << "========================================\n";
    cout << "       EARTH ANALOG ANALYZER\n";
    cout << "========================================\n";
    cout << "A first comparison between Earth and planetary regions.\n";

    vector<Profile> earthSites = readProfiles(earthFile, false);
    vector<Profile> references = readProfiles(referenceFile, true);

    if (earthSites.empty() || references.empty()) {
        cerr << "\nThe program could not load the data. Check the two input files.\n";
        return 1;
    }

    cout << "\nLoaded " << earthSites.size() << " Earth locations and "
         << references.size() << " reference regions.\n";

    cout << "\nPlease keep in mind:\n";
    cout << "- Missing measurements are skipped, not counted as zero.\n";
    cout << "- The score describes range overlap only. It is not a probability.\n";
    cout << "- A wide reference range can cover a whole Earth range without\n";
    cout << "  the two places being very similar.\n";
    cout << "- The result depends on the quality of the input data.\n";

    bool temperatureCompared = false;
    bool slopeOrReliefCompared = false;

    for (const Profile& reference : references) {
        cout << "\n\n========================================\n";
        cout << reference.target << " - " << reference.name << "\n";
        cout << "Reference source: " << reference.source << "\n";
        cout << "========================================\n";

        vector<Result> results;

        for (const Profile& site : earthSites) {
            Result result = analyzeSite(site, reference);
            printResult(site, reference, result);

            if (result.criteria[0].comparable) temperatureCompared = true;
            if (result.criteria[1].comparable || result.criteria[2].comparable) {
                slopeOrReliefCompared = true;
            }

            results.push_back(result);
        }

        printRanking(results);
    }

    // The supplied Mars and Moon files currently have temperature only.
    if (temperatureCompared && !slopeOrReliefCompared) {
        cout << "\nOne last note: temperature is the only measurement available on both\n";
        cout << "sides for these reference regions. Slope and local relief are missing\n";
        cout << "from the Mars and Moon data, so the order above is temperature only.\n";
    }

    return 0;
}
