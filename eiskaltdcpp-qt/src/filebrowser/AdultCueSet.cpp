/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "filebrowser/AdultCueSet.h"

#include <QStringView>

#include <algorithm>

namespace {

/**
 * Cues safe as plain substrings: age tags, distinctive invented words,
 * site/studio brands. Latin "porn" and Cyrillic "порн" cover most
 * derivatives (pornhub, porno, порнуха).
 */
const char *const kSubstringCues[] = {
    "[18+]", "[19+]", "[adult]", "[porn]", "[xxx]", "18+", "+18",
    "nsfw", "onlyfans", "fansly", "manyvids", "clips4sale",
    "porn", "xxx", "порн",
    // hentai / anime tags
    "hentai", "futanari", "yaoi", "ecchi", "eroge", "ahegao", "oppai",
    // tube sites / trackers / cam sites
    "pornhub", "xvideos", "xhamster", "redtube", "youporn", "spankbang",
    "xnxx", "txxx", "eporner", "hqporner", "porntrex", "beeg",
    "motherless.com", "thisvid", "empornium", "redgifs", "erome",
    "chaturbate", "stripchat", "bongacams", "camsoda", "myfreecams",
    "camster", "camgirl", "camwhore",
    // JAV uncensored labels & aggregators (kanji: uncensored, creampie,
    // big breasts, married woman; hangul: 야동 "adult video")
    "caribbeancom", "heyzo", "1pondo", "10musume", "pacopacomama",
    "tokyohot", "kin8tengoku", "fc2ppv", "javbus", "javlibrary",
    "javmost", "javhd", "sukebei",
    "無修正", "中出し", "巨乳", "人妻", "야동",
    // studios / networks (invented compound words)
    "brazzers", "bangbros", "realitykings", "naughtyamerica",
    "digitalplayground", "evilangel", "wickedpictures", "newsensations",
    "julesjordan", "woodmancastingx", "privatecastings", "legalporno",
    "analvids", "czechav", "czechcasting", "czechamateurs", "czechtaxi",
    "girlsdoporn", "girlsdotoys", "exploitedteens", "exploitedcollegegirls",
    "netvideogirls", "backroomcastingcouch", "brandnewamateurs",
    "mommysgirl", "girlsway", "webyoung", "familystrokes", "daughterswap",
    "sislovesme", "brattysis", "momsteachsex", "puretaboo", "missax",
    "elegantangel", "devilsfilm", "tushyraw", "blackedraw",
    "teamskeet", "mofos", "twistys", "propertysex", "fakehub", "faketaxi",
    "publicagent", "girlfriendsfilms", "sweetheartvideo", "milehighmedia",
    "nubiles", "nubilefilms", "wowgirls", "hegre", "dogfart",
    "blacksonblondes", "penthousegold", "seancody", "corbinfisher",
    "chaosmen", "cockyboys", "peterfever", "randyblue", "lucasentertainment",
    "falconstudios", "activeduty", "helixstudios", "belamionline",
    "guysinsweatpants", "fraternityx", "21sextury", "21naturals",
    "joybear", "vivthomas", "marcdorcel", "dorcelclub",
    "ultrafilms", "sacana"
};

/**
 * Single tokens matched on word boundaries. Unicode-aware \w makes the
 * boundaries work for Cyrillic too, so «анализ», «канал», «панспермия»,
 * «сперматозоид» never match. Entries are regex fragments: bare literals
 * for strict forms, explicit \w* where Russian inflections must match.
 */
const char *const kBoundedCues[] = {
    // acts / anatomy (Latin)
    "anal", "sex", "creampie", "blowjob", "handjob", "footjob", "deepthroat",
    "gangbang", "bukkake", "cumshot", "femdom", "cuckold", "pegging",
    "fisting", "squirting", "upskirt", "downblouse", "voyeur", "exhibitionist",
    "dildo", "buttplug", "vibrator", "orgasm", "masturbation", "orgy",
    "threesome", "milf", "softcore", "shemale", "tranny", "tgirls", "bdsm",
    "dominatrix", "gloryhole", "swingers", "nude", "nudism", "nudist",
    "lesbian", "playboy", "hustler", "emmanuelle", "slut\\w*", "throatfuck\\w*",
    // mononym stage names
    "stoya", "amouranth",
    // Russian: inflected stems carry \w*, strict forms stay bare
    "секс(?:а|е|у|ом|и)?", "эротик\\w*", "интим\\w*", "хентай\\w*",
    "минет\\w*", "анал", "сперм(?:а|ы|ой)?", "бдсм", "порев\\w*",
    "групповух\\w*", "свингер\\w*", "вуайер\\w*", "лесбиян\\w*",
    "страпон\\w*", "фистинг\\w*", "ню", "эмм?ануэль\\w*"
};

/**
 * Multi-word cues; in file names the words may be split by space, dot,
 * underscore or dash ("Riley.Reid", "mia_khalifa").
 */
const char *const kPhraseCues[] = {
    // well-known performers
    "riley reid", "mia khalifa", "lana rhoades", "abella danger",
    "angela white", "ava addams", "brandi love", "lisa ann", "nicole aniston",
    "phoenix marie", "rachel starr", "alexis texas", "kagney linn karter",
    "dani daniels", "gianna michaels", "jayden jaymes", "julia ann",
    "keisha grey", "kendra lust", "lexi belle", "madison ivy", "nina hartley",
    "peter north", "sunny leone", "tera patrick", "jesse jane", "kayden kross",
    "joanna angel", "august ames", "autumn falls", "emily willis",
    "gabbie carter", "gia derza", "karlee grey", "kendra sunderland",
    "lena paul", "little caprice", "markus dupree", "mick blue", "ramon nomar",
    "toni ribas", "erik everhard", "nacho vidal", "mike adriano", "james deen",
    "johnny sins", "xander corvus", "keiran lee", "scott nails",
    "charles dera", "manuel ferrara", "rocco siffredi", "ron jeremy",
    "sasha grey", "tori black", "asa akira", "mia malkova", "adriana chechik",
    "remy lacroix", "valentina nappi", "anissa kate", "kelsi monroe",
    "abigail mac", "blake blossom", "cory chase", "chanel preston",
    "christy mack", "eva elfie", "violet myers", "karma rx", "lela star",
    "moriah mills", "savannah bond", "jia lissa", "belle delphine",
    // famous JAV performers
    "hitomi tanaka", "anri okita", "maria ozawa", "yua mikami",
    "shoko takahashi", "eimi fukada", "saika kawakita", "ai uehara",
    // performers found in real shares
    "skin diamond", "simony diamond", "maddy o'reilly", "throat fuck",
    // erotic cinema classics & directors
    "tinto brass", "тинто брасс", "debbie does dallas", "deep throat",
    "behind the green door", "baise moi"
};

/** Mainstream titles whose generic cue («sex», «секс») must not flag them. */
const char *const kExclusionTitles[] = {
    "sex pistols", "sex and the city", "sex education",
    "секс в большом городе", "сексуальное просвещение"
};

/** JAV label codes: LABEL-12345, FC2-PPV ids, 1pondo ids. Case-insensitive. */
const char *const kJavLabelCodes[] = {
    "JUR", "JUQ", "JUL", "ROE", "ACHJ", "SSIS", "MIDE", "IPX", "IPZ", "MIMK",
    "START", "ABP", "ABF", "ABW", "ADN", "STARS", "MIDV", "CAWD", "HND",
    "MEYD", "WAAA", "DASS", "PPPE", "SONE", "FAD", "FSDSS", "SDDE", "SDMU",
    "RCT", "MIAA", "MIFD", "ATID", "CEAD", "CEMD", "DLDSS", "DVAJ", "EBOD",
    "EYAN", "GANA", "HMN", "HODV", "MKMP", "MXGS", "MXSPS", "NSFS", "OFJE",
    "PRED", "RBD", "SIRO", "SIVR", "SQTE", "UMSO", "WANZ"
};

template <std::size_t N>
QStringList decode(const char *const (&cues)[N])
{
    QStringList out;
    out.reserve(int(N));
    for (const char *cue : cues)
        out << QString::fromUtf8(cue);
    return out;
}

QRegularExpression buildMaster(const QStringList &substrings, const QStringList &bounded,
                               const QStringList &phrases, const QStringList &javLabels)
{
    QStringList parts;

    parts << QStringLiteral("(?<!\\w)(?:%1)(?!\\w)").arg(bounded.join(QLatin1Char('|')));

    for (const QString &phrase : phrases) {
        const QStringList words = phrase.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QStringList escaped;
        escaped.reserve(words.size());
        for (const QString &word : words)
            escaped << QRegularExpression::escape(word);
        parts << QStringLiteral("(?<!\\w)%1(?!\\w)").arg(escaped.join(QStringLiteral("[\\s._\\-]+")));
    }

    QStringList subs;
    subs.reserve(substrings.size());
    for (const QString &cue : substrings)
        subs << QRegularExpression::escape(cue);
    parts << QStringLiteral("(?:%1)").arg(subs.join(QLatin1Char('|')));

    parts << QStringLiteral("\\b(?:%1)-\\d{2,5}\\b"
                            "|\\bFC2[ ._\\-]*(?:PPV)?[ ._\\-]*\\d{6,8}\\b"
                            "|\\b1pon(?:do)?-\\d+\\b").arg(javLabels.join(QLatin1Char('|')));

    return QRegularExpression(parts.join(QLatin1Char('|')),
                              QRegularExpression::CaseInsensitiveOption
                                      | QRegularExpression::UseUnicodePropertiesOption);
}

} // namespace

AdultCueSet::AdultCueSet()
    : m_substrings(decode(kSubstringCues)),
      m_bounded(decode(kBoundedCues)),
      m_phrases(decode(kPhraseCues)),
      m_exclusions(decode(kExclusionTitles)),
      m_javLabels(decode(kJavLabelCodes))
{
    m_master = buildMaster(m_substrings, m_bounded, m_phrases, m_javLabels);
    m_brand = QRegularExpression(
            QStringLiteral("(?:VIXEN|DEEPER|BLACKED|SLAYED)[ ._\\-]*(?:\\d{2}\\.\\d{2}\\.\\d{2}|RAW)\\b"));
    buildTriggers();
}

const AdultCueSet &AdultCueSet::instance()
{
    static const AdultCueSet set;
    return set;
}

bool AdultCueSet::matches(const QString &hay) const
{
    if (m_brand.match(hay).hasMatch())
        return true;
    const QString lowered = hay.toLower();
    if (!mayMatch(lowered))
        return false;
    if (!m_master.match(lowered).hasMatch())
        return false;
    for (const QString &exclusion : m_exclusions) {
        if (lowered.contains(exclusion))
            return false;
    }
    return true;
}

bool AdultCueSet::mayMatch(const QString &lowered) const
{
    const QStringView view(lowered);
    const int size = view.size();
    for (int i = 0; i < size; ++i) {
        const Range &range = m_table[view.at(i).unicode()];
        for (int idx = range.first; idx < range.second; ++idx) {
            if (view.mid(i).startsWith(m_literals[idx]))
                return true;
        }
    }
    return false;
}

/** Every cue contains a literal trigger; index them by first UTF-16 unit. */
void AdultCueSet::buildTriggers()
{
    auto add = [this](const QString &literal) {
        if (!literal.isEmpty())
            m_literals.push_back(literal);
    };

    for (const QString &cue : m_substrings)
        add(cue);

    for (const QString &token : m_bounded) {
        // Leading literal run of a fragment: "эротик\w*" -> "эротик".
        QString stem;
        for (const QChar ch : token) {
            if (ch == QLatin1Char('\\') || ch == QLatin1Char('(')
                    || ch == QLatin1Char('|') || ch == QLatin1Char(')'))
                break;
            stem += ch;
        }
        add(stem);
    }

    for (const QString &phrase : m_phrases) {
        const QStringList words = phrase.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &word : words)
            add(word);
    }

    for (const QString &label : m_javLabels)
        add(label.toLower());
    add(QStringLiteral("fc2"));
    add(QStringLiteral("1pon"));

    std::sort(m_literals.begin(), m_literals.end());
    m_literals.erase(std::unique(m_literals.begin(), m_literals.end()), m_literals.end());
    for (int idx = 0; idx < int(m_literals.size()); ++idx) {
        auto &range = m_table[m_literals[idx].at(0).unicode()];
        if (range.second == 0)
            range.first = idx;
        range.second = idx + 1;
    }
}
