/**
 * Source parts info.
 * Meaningful only when Source::FLAG_PARTIAL is set.
 */
class PartialSource : public FastAlloc<PartialSource>, public intrusive_ptr_base<PartialSource> {
public:
    PartialSource(const string& aMyNick, const string& aHubIpPort, const string& aIp, const string& udp) :
        myNick(aMyNick), hubIpPort(aHubIpPort), ip(aIp), udpPort(udp), nextQueryTime(0), pendingQueryCount(0) { }

    ~PartialSource() { }

    typedef dcpp::intrusive_ptr<PartialSource> Ptr;

    GETSET(PartsInfo, partialInfo, PartialInfo);
    GETSET(string, myNick, MyNick);                 // for NMDC support only
    GETSET(string, hubIpPort, HubIpPort);
    GETSET(string, ip, Ip);
    GETSET(string, udpPort, UdpPort);
    GETSET(uint64_t, nextQueryTime, NextQueryTime);
    GETSET(uint8_t, pendingQueryCount, PendingQueryCount);
};

class Source : public Flags {
public:
    enum {
        FLAG_NONE = 0x00,
        FLAG_FILE_NOT_AVAILABLE = 0x01,
        FLAG_PASSIVE = 0x02,
        FLAG_REMOVED = 0x04,
        FLAG_CRC_FAILED = 0x08,
        FLAG_CRC_WARN = 0x10,
        FLAG_NO_TTHF = 0x20,
        FLAG_BAD_TREE = 0x40,
        FLAG_NO_TREE = 0x80,
        FLAG_SLOW_SOURCE = 0x100,
        FLAG_PARTIAL    = 0x200,
        FLAG_NO_NEED_PARTS      = 0x250,
        FLAG_TTH_INCONSISTENCY  = 0x300,
        FLAG_UNTRUSTED = 0x400,
        FLAG_UNENCRYPTED = 0x450,
        FLAG_MASK = FLAG_FILE_NOT_AVAILABLE
        | FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN
        | FLAG_BAD_TREE | FLAG_NO_TREE | FLAG_SLOW_SOURCE | FLAG_TTH_INCONSISTENCY | FLAG_UNTRUSTED | FLAG_UNENCRYPTED
    };

    Source(const HintedUser& aUser) : user(aUser), partialSource(NULL) { }
    Source(const Source& aSource) : Flags(aSource), user(aSource.user), partialSource(aSource.partialSource) { }

    bool operator==(const UserPtr& aUser) const { return user == aUser; }
    PartialSource::Ptr& getPartialSource() { return partialSource; }

    GETSET(HintedUser, user, User);
    GETSET(PartialSource::Ptr, partialSource, PartialSource);
};
