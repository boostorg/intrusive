include(BoostTest OPTIONAL RESULT_VARIABLE HAVE_BOOST_TEST)

if(NOT HAVE_BOOST_TEST)
  return()
endif()

#Boost.Intrusive plus the libraries the tests include directly
set(BOOST_TEST_LINK_LIBRARIES Boost::intrusive Boost::container Boost::container_hash
    Boost::core Boost::move Boost::static_assert Boost::type_traits)

#Every .cpp file in this directory is a test, as in Jamfile.v2
file(GLOB tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach(test IN LISTS tests)
  boost_test(SOURCES ${test})
endforeach()
