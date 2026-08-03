#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class NtagType : uint8_t {
    Ntag213,
    Ntag215,
    Ntag216,
    UltralightEv1,
};

struct NtagSpec {
    NtagType type;
    uint16_t totalPages;
    uint16_t userPageStart;
    uint16_t userPageEnd;
    uint16_t configPageStart;
    uint16_t configPageEnd;
    uint16_t userBytes;
};

struct NdefRecord {
    enum class Kind : uint8_t { Uri, Text };
    Kind kind;
    std::string value;

    static NdefRecord uri(const std::string& value) { return {Kind::Uri, value}; }
    static NdefRecord text(const std::string& value) { return {Kind::Text, value}; }
};

struct NtagWritePlan {
    uint16_t firstPage = 0;
    uint16_t pageCount = 0;
    bool valid = false;
    bool requiresConfirmation = false;
    const char* warning = "";
};

class INtagPageTransport {
public:
    virtual ~INtagPageTransport() = default;
    virtual bool writePage(uint16_t page, const std::array<uint8_t, 4>& data) = 0;
    virtual bool readPage(uint16_t page, std::array<uint8_t, 4>& data) = 0;
};

struct NtagWriteExecution {
    bool completed = false;
    uint16_t pagesWritten = 0;
    const char* detail = "";
};

class NtagModel {
public:
    explicit NtagModel(NtagType type) : type_(type) {}

    static NtagSpec spec(NtagType type);
    static bool isUserPage(NtagType type, uint16_t page);
    static bool isConfigPage(NtagType type, uint16_t page);
    static bool isLockPage(NtagType type, uint16_t page);
    static std::vector<uint8_t> encodeNdef(const std::vector<NdefRecord>& records);

    NtagWritePlan buildWritePlan(uint16_t firstPage, uint16_t pageCount) const;
    NtagWriteExecution writePages(INtagPageTransport& transport,
                                  uint16_t firstPage,
                                  const std::vector<std::array<uint8_t, 4>>& pages,
                                  bool confirmIrreversible) const;
    bool setPassword(const std::array<uint8_t, 4>& password, uint16_t auth0Page);
    bool removePassword();
    bool passwordEnabled() const { return passwordEnabled_; }
    uint16_t auth0Page() const { return auth0Page_; }
    const char* irreversibleWarning() const;

private:
    NtagType type_;
    bool passwordEnabled_ = false;
    std::array<uint8_t, 4> password_{};
    uint16_t auth0Page_ = 0;
};

class IFeature;
IFeature* createNtagToolsFeature();
