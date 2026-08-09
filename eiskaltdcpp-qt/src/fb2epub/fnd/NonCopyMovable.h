#pragma once

#define NON_COPYABLE(NAME)                 \
public:                                    \
	NAME(const NAME&)            = delete; \
	NAME& operator=(const NAME&) = delete; \
                                           \
private:

#define NON_MOVABLE(NAME)                      \
public:                                        \
	NAME(NAME&&) noexcept            = delete; \
	NAME& operator=(NAME&&) noexcept = delete; \
                                               \
private:

// rule 5
#define NON_COPY_MOVABLE(NAME) NON_COPYABLE(NAME) NON_MOVABLE(NAME)

#define DEFAULT_COPY_MOVABLE(NAME)              \
public:                                         \
	NAME(const NAME&)                = default; \
	NAME& operator=(const NAME&)     = default; \
	NAME(NAME&&) noexcept            = default; \
	NAME& operator=(NAME&&) noexcept = default; \
                                                \
private:
