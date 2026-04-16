#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
using namespace std;

struct Request {
    enum class Type { Read, Write } type;
    uint32_t address;
    uint32_t writeData;
    int id;
};

static string hex32(uint32_t value) {
    stringstream ss;
    ss << "0x" << hex << uppercase << setw(8) << setfill('0') << value;
    return ss.str();
}

class MainMemory {
public:
    explicit MainMemory(int latencyCycles) : latency(latencyCycles) {}

    void initializeBlock(uint32_t blockAddress, const vector<uint32_t>& words) {
        blocks[blockAddress] = words;
    }

    void startRead(uint32_t blockAddress) {
        busy = true;
        isWrite = false;
        addr = blockAddress;
        cycles = latency;
    }

    void startWrite(uint32_t blockAddress, const vector<uint32_t>& data) {
        busy = true;
        isWrite = true;
        addr = blockAddress;
        writeBuf = data;
        cycles = latency;
    }

    void tick() {
        ready = false;
        if (!busy) return;

        if (--cycles == 0) {
            if (isWrite) {
                blocks[addr] = writeBuf;
            } else {
                if (!blocks.count(addr)) {
                    blocks[addr] = vector<uint32_t>(4, 0);
                }
                readBuf = blocks[addr];
            }
            ready = true;
            busy = false;
        }
    }

    bool isReady() const { return ready; }
    bool isBusy() const { return busy; }
    const vector<uint32_t>& getReadBuffer() const { return readBuf; }

private:
    int latency;
    bool busy = false, ready = false, isWrite = false;
    int cycles = 0;
    uint32_t addr = 0;
    vector<uint32_t> readBuf, writeBuf;
    unordered_map<uint32_t, vector<uint32_t>> blocks;
};

class CacheController {
public:
    enum class State { Idle, CompareTag, Allocate, WriteBack };

    struct Line {
        bool valid = false, dirty = false;
        uint32_t tag = 0;
        vector<uint32_t> words = vector<uint32_t>(4, 0);
    };

    CacheController(MainMemory& m, int idxBits = 2)
        : memory(m), size(1 << idxBits), lines(size) {}

    void enqueueRequest(const Request& r) { cpuQ.push(r); }

    bool done() const {
        return cpuQ.empty() && !hasActive && state == State::Idle;
    }

    void tick(int cycle) {
        memory.tick();

        cpuReady = false;
        memValid = false;
        memWrite = false;

        switch (state) {
            case State::Idle: idle(); break;
            case State::CompareTag: compare(); break;
            case State::Allocate: allocate(); break;
            case State::WriteBack: writeback(); break;
        }

        print(cycle);
    }

private:
    MainMemory& memory;
    int size;
    vector<Line> lines;
    queue<Request> cpuQ;

    bool hasActive = false;
    Request active;
    bool memRequestSent = false;

    State state = State::Idle;

    bool cpuReady = false;
    bool memValid = false;
    bool memWrite = false;
    uint32_t memAddr = 0;
    string action;

    struct Decoded { uint32_t tag, idx, off, block; };

    Decoded decode(uint32_t addr) {
        uint32_t off = (addr >> 2) & 3;
        uint32_t idx = (addr >> 4) & (size - 1);
        uint32_t tag = addr >> 6;
        uint32_t block = addr >> 4;
        return {tag, idx, off, block};
    }

    void idle() {
        action = "Waiting";
        if (!cpuQ.empty()) {
            active = cpuQ.front();
            cpuQ.pop();
            hasActive = true;
            state = State::CompareTag;
            action = "Fetched request";
        }
    }

    void compare() {
        auto d = decode(active.address);
        Line& l = lines[d.idx];

        if (l.valid && l.tag == d.tag) {
            cpuReady = true;
            action = "Hit";
            if (active.type == Request::Type::Write) {
                l.words[d.off] = active.writeData;
                l.dirty = true;
            }
            hasActive = false;
            state = State::Idle;
        } else {
            if (l.valid && l.dirty) {
                state = State::WriteBack;
                action = "Miss -> WriteBack";
            } else {
                state = State::Allocate;
                action = "Miss -> Allocate";
            }
            l.tag = d.tag;
        }
    }

    void writeback() {
        auto d = decode(active.address);
        Line& l = lines[d.idx];

        if (!memRequestSent) {
            memory.startWrite(d.block, l.words);
            memValid = true;
            memWrite = true;
            memRequestSent = true;
            action = "Writing back";
        }
        else if (memory.isReady()) {
            l.dirty = false;
            state = State::Allocate;
            memRequestSent = false; 
            action = "WriteBack done";
        }
    }

    void allocate() {
        auto d = decode(active.address);
        Line& l = lines[d.idx];

        if (!memRequestSent) {
            memory.startRead(d.block);
            memValid = true;
            memRequestSent = true;
            action = "Reading from memory";
        }
        else if (memory.isReady()) {
            l.words = memory.getReadBuffer();
            l.valid = true;
            l.dirty = false;

            if (active.type == Request::Type::Write) {
                l.words[d.off] = active.writeData;
                l.dirty = true;
            }

            cpuReady = true;
            hasActive = false;
            state = State::Idle;
            memRequestSent = false; 
            action = "Allocate done";
        }
    }

    void print(int c) {
        cout << "Cycle " << setw(2) << c
             << " | State=" << (int)state
             << " | CPU.ready=" << cpuReady
             << " | Mem.valid=" << memValid
             << " | Mem.write=" << memWrite
             << " | " << action << '\n';
    }
};

int main() {
    MainMemory mem(2);

    CacheController c(mem);

    vector<Request> seq = {
        {Request::Type::Read, 0x0, 0, 1},
        {Request::Type::Write, 0x40, 0xDEADBEEF, 2},
        {Request::Type::Read, 0x0, 0, 3}
    };

    for (auto& r : seq) c.enqueueRequest(r);

    int cycle = 0;
    while (!c.done() && cycle < 50) {
        c.tick(cycle++);
    }
}

