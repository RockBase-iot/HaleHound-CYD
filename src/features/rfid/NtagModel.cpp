#include "features/rfid/NtagModel.h"

#include <algorithm>

NtagSpec NtagModel::spec(NtagType type) {
    switch (type) {
    case NtagType::Ntag213:
        return {type, 45, 4, 39, 41, 44, 144};
    case NtagType::Ntag215:
        return {type, 135, 4, 129, 131, 134, 504};
    case NtagType::Ntag216:
        return {type, 231, 4, 225, 227, 230, 888};
    case NtagType::UltralightEv1:
        return {type, 20, 4, 15, 16, 19, 48};
    }
    return {type, 0, 0, 0, 0, 0, 0};
}

bool NtagModel::isUserPage(NtagType type, uint16_t page) {
    const auto limits = spec(type);
    return page >= limits.userPageStart && page <= limits.userPageEnd;
}

bool NtagModel::isConfigPage(NtagType type, uint16_t page) {
    const auto limits = spec(type);
    return page >= limits.configPageStart && page <= limits.configPageEnd;
}

bool NtagModel::isLockPage(NtagType type, uint16_t page) {
    const auto limits = spec(type);
    return page == 2U || page == 3U || page == limits.userPageEnd + 1U;
}

std::vector<uint8_t> NtagModel::encodeNdef(const std::vector<NdefRecord>& records) {
    std::vector<uint8_t> message;
    for (size_t index = 0; index < records.size(); ++index) {
        const auto& record = records[index];
        std::vector<uint8_t> type;
        std::vector<uint8_t> payload;
        if (record.kind == NdefRecord::Kind::Uri) {
            type.push_back('U');
            uint8_t prefix = 0x00;
            std::string remainder = record.value;
            if (remainder.rfind("https://", 0U) == 0U) {
                prefix = 0x04;
                remainder.erase(0U, 8U);
            } else if (remainder.rfind("http://", 0U) == 0U) {
                prefix = 0x03;
                remainder.erase(0U, 7U);
            }
            payload.push_back(prefix);
            payload.insert(payload.end(), remainder.begin(), remainder.end());
        } else {
            type.push_back('T');
            payload.push_back(0x02); // UTF-8, language code length 2
            payload.push_back('e');
            payload.push_back('n');
            payload.insert(payload.end(), record.value.begin(), record.value.end());
        }
        if (type.size() > 255U || payload.size() > 255U) return {};
        const bool first = index == 0U;
        const bool last = index + 1U == records.size();
        uint8_t header = 0x01; // TNF well-known
        if (first) header |= 0x80;
        if (last) header |= 0x40;
        header |= 0x10; // short record
        message.push_back(header);
        message.push_back(static_cast<uint8_t>(type.size()));
        message.push_back(static_cast<uint8_t>(payload.size()));
        message.insert(message.end(), type.begin(), type.end());
        message.insert(message.end(), payload.begin(), payload.end());
    }
    if (message.empty() || message.size() > 254U) return {};
    std::vector<uint8_t> encoded;
    encoded.reserve(message.size() + 3U);
    encoded.push_back(0x03); // NDEF TLV
    encoded.push_back(static_cast<uint8_t>(message.size()));
    encoded.insert(encoded.end(), message.begin(), message.end());
    encoded.push_back(0xFE); // terminator TLV
    return encoded;
}

NtagWritePlan NtagModel::buildWritePlan(uint16_t firstPage, uint16_t pageCount) const {
    const auto limits = spec(type_);
    NtagWritePlan plan;
    plan.firstPage = firstPage;
    plan.pageCount = pageCount;
    if (pageCount == 0U || limits.totalPages == 0U ||
        firstPage >= limits.totalPages ||
        static_cast<uint32_t>(firstPage) + pageCount > limits.totalPages) {
        plan.warning = "page range is outside tag capacity";
        return plan;
    }
    plan.valid = true;
    for (uint16_t page = firstPage; page < firstPage + pageCount; ++page) {
        if (isLockPage(type_, page)) {
            plan.requiresConfirmation = true;
            plan.warning = irreversibleWarning();
            break;
        }
    }
    return plan;
}

NtagWriteExecution NtagModel::writePages(
    INtagPageTransport& transport,
    uint16_t firstPage,
    const std::vector<std::array<uint8_t, 4>>& pages,
    bool confirmIrreversible) const {
    NtagWriteExecution execution;
    const auto plan = buildWritePlan(firstPage, static_cast<uint16_t>(pages.size()));
    if (!plan.valid) {
        execution.detail = plan.warning;
        return execution;
    }
    if (plan.requiresConfirmation && !confirmIrreversible) {
        execution.detail = "irreversible lock-page confirmation required";
        return execution;
    }
    for (size_t index = 0; index < pages.size(); ++index) {
        const uint16_t page = static_cast<uint16_t>(firstPage + index);
        if (!transport.writePage(page, pages[index])) {
            execution.detail = "page write failed; subsequent pages were not written";
            return execution;
        }
        std::array<uint8_t, 4> verify{};
        if (!transport.readPage(page, verify) || verify != pages[index]) {
            execution.detail = "page verification failed; subsequent pages were not written";
            return execution;
        }
        ++execution.pagesWritten;
    }
    execution.completed = true;
    execution.detail = "all pages written and verified";
    return execution;
}

bool NtagModel::setPassword(const std::array<uint8_t, 4>& password, uint16_t auth0Page) {
    const auto limits = spec(type_);
    if (auth0Page < limits.userPageStart || auth0Page >= limits.totalPages) return false;
    if (std::all_of(password.begin(), password.end(), [](uint8_t byte) { return byte == 0U; })) {
        return false;
    }
    password_ = password;
    auth0Page_ = auth0Page;
    passwordEnabled_ = true;
    return true;
}

bool NtagModel::removePassword() {
    password_.fill(0U);
    auth0Page_ = 0U;
    passwordEnabled_ = false;
    return true;
}

const char* NtagModel::irreversibleWarning() const {
    return "Writing lock bytes is irreversible; confirm twice before continuing";
}
