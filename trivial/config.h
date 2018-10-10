/*
    This is definitely not the best practice, but the config is currently small enough to keep it just in separate header.
*/

#pragma once

enum ESlugGeneratorType {
    RandomMongoAware,
    Sequential,
};

/*
    Mongo collections.
*/

const std::string URL_SHORTENER_DB = "url_shortener";
const std::string URL_SLUG_COLLECTION = "slug";

/*
    Slug-related.
*/

const std::string BASE_HOST = "http://localhost:18080/";
const uint32_t SLUG_LENGTH = 6;
const ESlugGeneratorType SLUG_GENERATOR_TYPE = ESlugGeneratorType::RandomMongoAware;

/*
    Daemon-related.
*/

const uint32_t DAEMON_LAUNCH_FREQUENCY = 90;
const uint32_t DAEMON_ITERATION_DURATION = 30;

/*
    Deploy.
*/

const uint32_t DEPLOY_PORT = 18080;

