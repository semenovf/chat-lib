////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ContactManagerBuilder.hpp"
#include "pfs/fmt.hpp"
#include "pfs/iterator.hpp"
#include "pfs/memory.hpp"

// Data generated by `https://www.mockaroo.com`
namespace {
    struct sample_data_t {
        char const * first_name;
        char const * last_name;
        char const * last_activity;
    } SAMPLE_DATA[] = {
          {"Trevar"   , "Ferrand"     , "2021-03-29T16:47:48Z"}
        , {"Jammie"   , "Crumbleholme", "2021-03-22T22:32:17Z"}
        , {"Kayne"    , "Fenney"      , "2021-03-14T06:58:33Z"}
        , {"Marco"    , "Janeway"     , "2021-02-02T15:48:41Z"}
        , {"Noreen"   , "Goter"       , "2021-02-25T20:25:18Z"}
        , {"Danita"   , "Cuardall"    , "2020-12-27T12:24:08Z"}
        , {"Abagael"  , "Jachimiak"   , "2021-08-25T00:38:24Z"}
        , {"Wendeline", "Leed"        , "2021-11-13T16:35:56Z"}
        , {"Paula"    , "Ivanyushkin" , "2020-12-30T03:59:52Z"}
        , {"Antonie"  , "Huntington"  , "2021-10-04T05:25:32Z"}
        , {"Armando"  , "Barnett"     , "2021-06-03T04:36:39Z"}
        , {"Natale"   , "Rainon"      , "2020-12-27T15:19:23Z"}
        , {"Vicky"    , "Algate"      , "2021-10-07T03:30:24Z"}
        , {"Matti"    , "Lantuffe"    , "2021-12-13T03:47:10Z"}
        , {"Merline"  , "Aimable"     , "2021-02-13T20:03:03Z"}
        , {"Bunnie"   , "Wager"       , "2021-02-08T14:38:34Z"}
        , {"Lauraine" , "Standon"     , "2021-08-08T17:24:06Z"}
        , {"Virgil"   , "Keyzor"      , "2021-03-17T20:50:23Z"}
        , {"Blake"    , "Tremmel"     , "2021-01-17T11:20:41Z"}
        , {"Consuela" , "Westwick"    , "2021-06-06T20:17:05Z"}
        , {"Elene"    , "Doe"         , "2021-09-02T09:45:31Z"}
        , {"Fred"     , "Lowthian"    , "2021-04-30T02:59:43Z"}
        , {"Leora"    ,"Leathe"       , "2021-12-19T00:06:12Z"}
        , {"Glynn","Groucock","2021-02-10T14:13:51Z"}
        , {"Roddie","Hallahan","2021-04-20T23:38:12Z"}
        , {"Worthy","Applewhite","2021-04-29T18:21:36Z"}
        , {"Hildagarde","Paroni","2021-05-27T07:37:22Z"}
        , {"Con","Rudgerd","2021-04-13T12:22:41Z"}
        , {"Wood","Chewter","2021-07-12T05:59:02Z"}
        , {"Janot","Matzen","2021-06-05T14:34:59Z"}
        , {"Selby","Simonnot","2021-03-21T00:30:18Z"}
        , {"Siward","Lawty","2021-07-01T07:35:41Z"}
        , {"Orelie","Filer","2021-06-05T01:52:07Z"}
        , {"Suzanna","Vanacci","2021-12-09T17:50:06Z"}
        , {"Jesse","Burgett","2021-07-09T17:25:09Z"}
        , {"Brander","Truce","2021-07-15T09:55:36Z"}
        , {"Shelia","Nurny","2021-08-21T19:37:06Z"}
        , {"Arlinda","Hargrove","2021-02-28T12:10:52Z"}
        , {"Tallulah","Schukraft","2021-12-05T09:37:53Z"}
        , {"Cris","Axcel","2021-07-02T10:41:49Z"}
        , {"Jedediah","Shuxsmith","2021-09-27T17:24:09Z"}
        , {"Courtnay","O' Mahony","2021-12-03T01:16:37Z"}
        , {"Patricio","Onion","2021-01-09T14:53:28Z"}
        , {"Laney","Sawter","2021-06-01T05:33:50Z"}
        , {"Janaya","Kilfedder","2020-12-27T15:32:42Z"}
        , {"Datha","Hendron","2021-06-02T10:39:15Z"}
        , {"Vita","Pere","2021-09-26T10:09:09Z"}
        , {"Janis","Schimoni","2021-12-01T07:45:34Z"}
        , {"Lucilia","Haslock","2021-06-23T11:40:31Z"}
        , {"Margaretta","Leisk","2021-05-24T17:37:19Z"}
        , {"Fritz","Gothliff","2021-12-25T05:17:23Z"}
        , {"Hillie","MacKain","2021-04-11T20:12:59Z"}
        , {"Romonda","Sprott","2021-01-10T14:36:52Z"}
        , {"Mendie","Blaxeland","2021-01-25T07:51:24Z"}
        , {"Giuditta","Mewburn","2021-05-13T17:21:50Z"}
        , {"Codie","Lynnitt","2021-12-01T13:00:18Z"}
        , {"Vivianne","Judge","2021-09-26T17:06:48Z"}
        , {"Mady","Bowling","2021-01-21T01:27:53Z"}
        , {"Caryl","Hehl","2021-09-02T19:04:52Z"}
        , {"Stormy","Haymes","2021-03-30T22:36:03Z"}
        , {"Nessy","Sprankling","2021-08-11T07:35:37Z"}
        , {"Lyn","Camocke","2021-07-15T14:02:52Z"}
        , {"Mitzi","Claeskens","2021-12-11T23:47:23Z"}
        , {"Alverta","Loughran","2021-07-10T09:00:30Z"}
        , {"Kellia","Balthasar","2021-03-04T18:18:40Z"}
        , {"Hester","Boyan","2021-07-09T13:55:23Z"}
        , {"Isidoro","Barge","2021-03-08T22:57:52Z"}
        , {"Ezmeralda","Searsby","2021-11-08T11:22:22Z"}
        , {"Barton","Dakhov","2021-05-09T14:44:48Z"}
        , {"Roze","Duffrie","2021-07-29T00:59:45Z"}
        , {"Cointon","Smissen","2021-01-21T21:22:36Z"}
        , {"Drusilla","Saunders","2021-11-18T02:52:38Z"}
        , {"Reese","Studdard","2021-06-23T19:28:02Z"}
        , {"Ardra","Vezey","2021-12-06T17:58:34Z"}
        , {"Buffy","Muller","2021-04-08T23:38:11Z"}
        , {"Alden","Pionter","2021-10-13T08:06:59Z"}
        , {"Thorpe","Turnock","2021-06-01T20:45:45Z"}
        , {"Cristine","Beadon","2021-10-25T01:08:25Z"}
        , {"Isabella","Wisham","2021-03-14T20:12:29Z"}
        , {"Gunilla","Hutchin","2020-12-28T15:06:46Z"}
        , {"Faye","Petkens","2021-05-20T21:02:29Z"}
        , {"Agatha","Pollastrone","2021-07-23T16:03:14Z"}
        , {"Sheridan","Nystrom","2021-05-30T13:18:36Z"}
        , {"Scott","Vaux","2021-03-04T16:26:40Z"}
        , {"Brooks","Lippo","2021-10-11T20:32:43Z"}
        , {"Padraic","Paulsson","2021-03-23T07:57:23Z"}
        , {"Dulci","Furneaux","2021-11-20T12:11:45Z"}
        , {"Lion","Satterly","2021-02-06T17:09:35Z"}
        , {"Brandy","Giraldon","2021-06-12T15:17:32Z"}
        , {"Venus","Sprosson","2021-01-17T06:04:36Z"}
        , {"Ameline","Crocket","2021-05-06T13:52:31Z"}
        , {"Ransom","Bouskill","2021-08-15T13:24:04Z"}
        , {"Janice","Rebichon","2021-02-27T13:49:57Z"}
        , {"Shawn","Ebbles","2021-02-13T00:19:19Z"}
        , {"Brunhilda","Kneaphsey","2021-08-22T14:45:40Z"}
        , {"Lotta","Burchall","2021-12-26T01:30:09Z"}
        , {"Marc","Bliben","2021-07-08T01:05:19Z"}
        , {"Gilbert","Croci","2021-07-13T15:07:48Z"}
        , {"Sig","Leeburn","2021-10-26T20:26:16Z"}
        , {"Isadora","Cheavin","2021-02-28T04:35:18Z"}
    };
} // namespace

struct forward_iterator : public pfs::iterator_facade<
          pfs::forward_iterator_tag
        , forward_iterator
        , chat::contact::contact, sample_data_t *, chat::contact::contact>
{
    sample_data_t * _p;
    forward_iterator (sample_data_t * p) : _p(p) {}

    reference ref ()
    {
        chat::contact::contact c;
        c.id    = pfs::generate_uuid();
        c.alias = _p->last_name;
        c.type  = chat::chat_enum::person;
        return c;
    }

    pointer ptr () { return _p; }
    void increment (difference_type) { ++_p; }
    bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
};

std::unique_ptr<ContactManagerBuilder::type> ContactManagerBuilder::operator () ()
{
    namespace fs = pfs::filesystem;

    auto on_failure = [] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
    };

    auto contactListPath = fs::temp_directory_path() / "messenger";

    if (!fs::exists(contactListPath)) {
        std::error_code ec;

        if (!fs::create_directory(contactListPath, ec)) {
            on_failure(fmt::format("Create directory failure: {}: {}"
                , pfs::filesystem::utf8_encode(contactListPath)
                , ec.message()));
            return nullptr;
        }
    }

    contactListPath /= "contact_list.db";

    auto dbh = chat::persistent_storage::sqlite3::make_handle(contactListPath
        , true, on_failure);

    if (!dbh)
        return nullptr;

    auto contactManager = pfs::make_unique<ContactManagerBuilder::type>(dbh, on_failure);

    if (!*contactManager)
        return nullptr;

    contactManager->wipe();

    auto & contacts = contactManager->contacts();

    auto success = contacts.add(forward_iterator{SAMPLE_DATA}
            , forward_iterator{SAMPLE_DATA + sizeof(SAMPLE_DATA)/sizeof(SAMPLE_DATA[0])});

    if (!success)
        return nullptr;

    return contactManager;
}
