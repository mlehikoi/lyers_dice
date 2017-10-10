#include "helpers.hpp"
#include "engine.hpp"
#include "json.hpp"

#include <unordered_set>

#include "gtest/gtest.h"

#include <rapidjson/document.h>

#include <cstdio>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;

namespace {

class AtEnd
{
    std::function<void()> f_;
public:
    AtEnd(const std::function<void()>& f) : f_{f} {}
    AtEnd(AtEnd&&) = delete;
    ~AtEnd() { f_(); }
};

inline std::string tmpName(const char* prefix)
{
    char tmp[PATH_MAX];
    std::strcpy(tmp, prefix);
    std::strcat(tmp, "XXXXXX");
    auto f = ::mkstemp(tmp);
    ::close(f);
    return tmp;
}

inline bool fileExists(const std::string& filename) 
{
    struct stat fileInfo;
    return ::stat(filename.c_str(), &fileInfo) == 0;
}


TEST(UuidTest, All) {
    std::unordered_set<std::string> s;
    
    for (int i = 0; i < 1'000; ++i)
    {
        auto id = dice::uuid();
        EXPECT_EQ(36, id.size());
        EXPECT_TRUE(s.insert(id).second);
    }
    EXPECT_EQ(1'000, s.size());
}

TEST(SlurpTest, All) {
    auto data = dice::slurp("../hex.dat");
    EXPECT_EQ(4, data.size());
    EXPECT_EQ('1', data[0]);
    EXPECT_EQ('2', data[1]);
    EXPECT_EQ('\0', data[2]);
    EXPECT_EQ('4', data[3]);
    
    data = dice::slurp("not-existing.txt");
    EXPECT_EQ(0, data.size());
}

TEST(ExtensionTest, All) {
    EXPECT_EQ("jpg", dice::getExtension("foo.jpg"));
    EXPECT_EQ("jpeg", dice::getExtension("foo.jpeg"));
    EXPECT_EQ("png", dice::getExtension("foo.jpg.png"));
}

TEST(ContentTypeTest, All) {
    EXPECT_EQ("image/jpeg", dice::getContentType("foo.jpg"));
    EXPECT_EQ("text/html; charset=utf-8", dice::getContentType("index.html"));
    EXPECT_EQ("image/png", dice::getContentType("foo.jpg.png"));
}

TEST(EngineTest, Login) {
    dice::Engine e{""};
    auto anon = R"#(
        {"name": "anon"}
    )#";
    auto result = e.login(anon);
    rapidjson::Document doc;
    doc.Parse(result.c_str());
    EXPECT_TRUE(doc["success"].GetBool());

    result = e.login(anon);
    doc.SetObject();
    doc.Parse(result.c_str());
    EXPECT_FALSE(doc["success"].GetBool());
}

TEST(EngineTest, CreateGameInvalidId) {
    dice::Engine e{""};
    auto game = R"#(
{
    "playerId": "000",
    "game": "game"
}
    )#";
    auto result = e.createGame(game);

    rapidjson::Document doc;
    doc.Parse(result.c_str());
    EXPECT_FALSE(doc["success"].GetBool());
    EXPECT_STREQ("NO_PLAYER", doc["error"].GetString());
}

TEST(EngineTest, CreateGame) {
    dice::Engine e{""};
    auto anon = R"#(
        {"name": "anon"}
    )#";
    auto result = e.login(anon);
    rapidjson::Document doc;
    doc.Parse(result.c_str());
    EXPECT_TRUE(doc["success"].GetBool());
    std::string id = doc["playerId"].GetString();

    auto game = json::Json({
        {"playerId", id},
        {"game", "game"}
    }).str();
    result = e.createGame(game);
    cout << "Result: " << result << endl;
    doc.Parse(result.c_str());
    EXPECT_TRUE(doc["success"].GetBool());
    
    auto game2 = json::Json({
        {"playerId", id},
        {"game", "game2"}
    }).str();
    result = e.createGame(game2);
    cout << "Result: " << result << endl;
    doc.Parse(result.c_str());
    EXPECT_FALSE(doc["success"].GetBool());
    EXPECT_STREQ("ALREADY_JOINED", doc["error"].GetString());

    // Try to create the same name game again
    result = e.login(json::Json({"name", "anon2"}).str());
    doc.Parse(result.c_str());
    EXPECT_TRUE(doc["success"].GetBool());
    cout << "Result: " << result << endl;
    id = doc["playerId"].GetString();
    
    auto game3 = json::Json({
        {"playerId", id},
        {"game", "game"}
    }).str();
    result = e.createGame(game3);
    cout << "Result: " << result << endl;
    doc.Parse(result.c_str());
    EXPECT_FALSE(doc["success"].GetBool());
    EXPECT_STREQ("GAME_EXISTS", doc["error"].GetString());
    
    cout << "Games: " << endl << e.getGames() << endl;
    doc = dice::parse(e.getGames());
    auto players = doc["game"].GetArray();
    EXPECT_EQ(1, players.Size());
    EXPECT_STREQ("anon", players[0].GetString());
}

TEST(EngineTest, Load) {
    dice::Engine e{"../test-game.json"};
    
    const auto doc = dice::parse(e.getGames());
    //dice::prettyPrint(doc);
    auto players = doc["final"].GetArray();
    EXPECT_EQ(2, players.Size());
    EXPECT_STREQ("joe", players[0].GetString());
    EXPECT_STREQ("mary", players[1].GetString());
}

TEST(EngineTest, Save) {
    auto tmp = tmpName("./.json");
    //AtEnd ae{[tmp]{ std::remove(tmp.c_str()); }};
    const auto origData = dice::slurp("../test-game.json");
    dice::dump(tmp, origData);

    dice::Engine e{tmp};
    EXPECT_TRUE(fileExists(tmp));
    std::remove(tmp.c_str());
    EXPECT_FALSE(fileExists(tmp));
    
    e.save();
    EXPECT_TRUE(fileExists(tmp));
    const auto doc = dice::parse(dice::slurp(tmp));
    EXPECT_TRUE(doc.IsObject());
    
    EXPECT_STREQ("joe", doc["1"]["name"].GetString());
    EXPECT_STREQ("final", doc["1"]["game"].GetString());
    
    EXPECT_STREQ("mary", doc["2"]["name"].GetString());
    EXPECT_STREQ("final", doc["2"]["game"].GetString());
    
    EXPECT_STREQ("ken", doc["3"]["name"].GetString());
    EXPECT_FALSE(doc["3"].HasMember("game"));
    
    //EXPECT_TRUE(doc["mary"].IsObject());
    //EXPECT_STREQ("1", doc["mary"]["name"].GetString());
    
    //EXPECT_TRUE(doc["joe"].IsObject());
    //EXPECT_STREQ("1", doc["joe"]["name"].GetString());
    
    //EXPECT_EQ(newData, origData);
    //const std::string tmp{std::tmpnam(nullptr)};
    //dice::dump(tmp);
}

} // Unnamed namespace