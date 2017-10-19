#pragma once
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

class IDice
{
public:
    virtual int roll() const = 0;
};
class Dice : public IDice
{
public:
    static auto& instance()
    {
        static Dice dice;
        return dice;
    }

    int roll() const override
    {
        static std::random_device r;
        static std::default_random_engine e1(r());
        std::uniform_int_distribution<int> uniform_dist(1, 6);
        return uniform_dist(e1);
    }
};


class Player
{
    std::string name_;
    std::vector<int> hand_;
    const IDice& diceRoll_;
public:
    Player(const std::string& name, const IDice& diceRoll)
      : name_{name},
        hand_(5),
        diceRoll_{diceRoll}
    {
        //roll();
        //std::cout << "dice: " << dice_.size() << std::endl;
        //for (const auto& d : dice_) std::cout << d << std::endl;
    }
    
    void roll()
    {
        for (auto& d : hand_)
        {
            d = diceRoll_.roll();
        }
    }
    
    void remove(std::size_t adjustment)
    {
        const auto size = hand_.size();
        hand_.resize(size - std::min(size, adjustment));
    }
    
    const auto& hand() const { return hand_; }
    
    void serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& w, const std::string& player) const
    {
        w.StartObject();
        w.Key("name");
        w.String(name_.c_str());
        w.Key("hand");
        w.StartArray();
        for (auto d : hand_)
        {
            w.Int(player.empty() || name_ == player ? d : 0);
        }
        w.EndArray();
        w.EndObject();
    }
    
    void serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& w,
                   const std::tuple<int, bool, bool>& result) const
    {
        w.StartObject();
        w.Key("name");
        w.String(name_.c_str());

        w.Key("adjustment"); w.Int(std::get<0>(result));
        w.Key("winner"); w.Bool(std::get<1>(result));
        w.Key("loser"); w.Bool(std::get<2>(result));

        w.Key("hand");
        w.StartArray();
        for (auto d : hand_)
        {
            w.Int(d);
        }
        w.EndArray();

        w.EndObject();
    }
    
    const auto& name() const { return name_; }
    bool isPlaying() const { return !hand_.empty(); }
};


class Game
{
    std::string game_;
    std::vector<Player> players_;
    int round_;
    int turn_;
    Bid currentBid_;
    bool roundStarted_;
    const IDice& diceRoll_;
    // enum State
    // {
    //     GAME_NOT_STARTED,
    //     GAME_STARTED,
    //     ROUND_STARTED,
    //     CHALLENGE,
    //     GAME_FINISHED
    // } state_;
    MAKE_ENUM(GameState,
        GAME_NOT_STARTED,
        GAME_STARTED,
        ROUND_STARTED,
        CHALLENGE,
        GAME_FINISHED
    );

    GameState state_;

    // Used for challenge
    const Player* bidder_;
    const Player* challenger_;
    
    auto getOffset() const
    {
        //if (state_ == CHALLENGE)
        {
            std::vector<int> commonHand;
            for (const auto p : players_)
            {
                commonHand.insert(commonHand.end(), p.hand().begin(), p.hand().end());
            }
            return currentBid_.challenge(commonHand);
        }
        return 0;
    }
    
    auto& currentPlayer() const { return players_[turn_]; }
    
    const auto challenger() const { return challenger_; }
    
    auto getResult(int offset, const Player& player) const
    {
        std::cout << player.name() << " ";
        std::cout << &player << " " << bidder_ << " " << challenger() << std::endl;
        if (&player == bidder_)
            return std::make_tuple(offset < 0 ? offset : 0, offset >= 0, offset < 0);
        if (&player == challenger())
            return std::make_tuple(offset >= 0 ? std::min(-1, -offset) : 0, offset < 0, offset >= 0);
        return std::make_tuple(offset == 0 ? -1 : 0, false, false);
    }
    
    void setTurn(const Player& player)
    {
        std::cout << "Set turn " << player.name() << std::endl;
        int i = 0;
        for (const auto& p : players_)
        {
            if (&p == &player)
            {
                turn_ = i;
                return;
            }
            ++i;
        }
    }

public:
    Game(const std::string& game, const IDice& diceRoll = Dice::instance())
      : game_{game},
        players_{},
        round_{},
        turn_{0},
        currentBid_{},
        roundStarted_{false},
        diceRoll_{diceRoll},
        state_{GAME_NOT_STARTED}
    {
    }

    void addPlayer(const std::string& player)
    {
        players_.push_back({player, diceRoll_});
    }

    auto players() const { return players_; }
    
    bool startGame()
    {
        if (state_ != GAME_NOT_STARTED && state_ != GAME_FINISHED)
            return false;
        state_ = GAME_STARTED;
        return true;
    }
    
    bool startRound()
    {
        switch (state_)
        {
        case CHALLENGE:
        {
            const auto offset = getOffset();
            for (auto& p : players_)
            {
                auto result = getResult(offset, p);
                p.remove(-std::get<0>(result));
            }
        }
        //[[clang::fallthrough]];
        case GAME_STARTED:
            state_ = ROUND_STARTED;
            currentBid_ = Bid{};
            for (auto& p : players_) p.roll();
            return true;
        default:
            return false;
        }
    }

    void nextPlayer()
    {
        ++turn_;
        auto nPlayers = players_.size();
        for (size_t i = 0; i < players_.size(); ++turn_)
        {
            turn_ %= nPlayers;
            if (players_[turn_].isPlaying()) return;
        }
        assert(false);
    }
    
    bool bid(const std::string& player, int n, int face)
    {
        //std::cout << "turn: " << turn_ << std::endl;
        //std::cout << player << " vs " << currentPlayer().name()  << std::endl;
        //@TODO return enum to indicate reason of failure
        std::cout << toString(state_) << " " << currentPlayer().name() << std::endl;
        if (state_ != ROUND_STARTED) return false;
        if (player == currentPlayer().name())
        {
            Bid bid{n, face};
            if (currentBid_ < bid)
            {
                currentBid_ = bid;
                //@TODO set bit to player
                bidder_ = &currentPlayer();
                nextPlayer();
                return true;
            }
            std::cout << "Too low bid" << std::endl;
            return false;
        }
        return false;
    }
    
    bool challenge(const std::string player)
    {
        if (currentBid_ == Bid{})
            return false;
        if (player != currentPlayer().name())
            return false;
        const auto offset = getOffset();
        
        // Is it done...
        int numPlayers = 0;
        for (const auto& p : players_)
        {
            auto result = getResult(offset, p);
            //std::cout << p.name() << " " << p.hand().size() << " "
            if (p.hand().size() > -std::get<0>(result)) ++numPlayers;
        }
        assert(numPlayers >= 1);
        state_ = numPlayers == 1 ? GAME_FINISHED : CHALLENGE;
        
        challenger_ = &currentPlayer();
        if (offset >= 0)
        {
            setTurn(*bidder_);
        }
        return true;
    }
    
    std::string getStatus(const std::string& player)
    {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> w{s};
        w.StartObject();
        w.Key("turn"); w.String(currentPlayer().name().c_str());
        w.Key("state"); w.String(toString(state_));

        w.Key("players");
        w.StartArray();
        
        const auto offset = getOffset();
        for (const auto& p : players_)
        {
            if (state_ == CHALLENGE || state_ == GAME_FINISHED)
            {
                auto result = getResult(offset, p);
                std::cout << p.name() << " " << offset << " " << std::get<0>(result) << std::endl;
                p.serialize(w, result);
            }
            else
            {
                p.serialize(w, player);
            }
        }
        w.EndArray();
        w.EndObject();
        return s.GetString();
    }
};

} // namespace game