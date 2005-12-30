#ifndef SERIALIZATION_H
#define SERIALIZATION_H 1


// This order is important
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>

#include <boost/serialization/export.hpp>

#define SER_BASE(ar,bc) \
    do { ar & boost::serialization::base_object<bc>(*this); } while (0)

#endif

