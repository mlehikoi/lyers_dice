#pragma once

#include "player.hpp"

#include "bid.hpp"
#include "json.hpp"
#include "helpers.hpp"

#include <rapidjson/prettywriter.h>

#include <random>
#include <string>
#include <vector>

namespace dice {

template<typename T>
inline T set(T& value, T newValue)
{
    T prev = value;
    value = newValue;
    return prev;
}

class Game
{
    std::string game_;
    std::vector<Player> players_;
    //int round_;
    int turn_;
    Bid currentBid_;
    //bool roundStarted_;
    const IDice& diceRoll_;
    enum State
    {
        GAME_NOT_STARTED,
        GAME_STARTED,
        ROUND_STARTED,
        CHALLENGE,
        GAME_FINISHED
    } state_;

    // Used for challenge
    const Player* bidder_;
    const Player* challenger_;
    
    int getOffset() const;

    static const char* toString(State state);
    static State fromString(const std::string& str);
    
    auto& currentPlayer() { return players_[static_cast<std::size_t>(turn_)]; }
    const auto& currentPlayer() const { return players_[static_cast<std::size_t>(turn_)]; }
    const auto& challenger() const { return challenger_; }
    
    std::tuple<int, bool, bool> getResult(int offset, const Player& player) const;
    
    void setTurn(const Player& player);

public:
    Game(const std::string& game, const IDice& diceRoll = Dice::instance());

    const auto& name() { return game_; }
    void addPlayer(const std::string& player);

    auto players() const { return players_; }
    
    bool startGame();
    
    RetVal startRound();
    void nextPlayer();
    RetVal bid(const std::string& player, int n, int face);
    RetVal challenge(const std::string player);
    std::string getStatus(const std::string& player);

    /**
     * Serialize engine state to give writer. If round is still in progress,
     * only given player's dice are "shown".
     *
     * @param w [out] state is serialized here
     * @param name [in] who's dice to show if round is in progress. If empty,
     *     show all dice.
     */
    void serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& w, const std::string& name) const;
    static std::shared_ptr<Game> fromJson(const rapidjson::Value& v);
};

} // namespace dice
