/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2026
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_PRIME_HOLDER_HPP
#define BOOST_INTRUSIVE_DETAIL_PRIME_HOLDER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

//Prime bucket counts used by the unordered containers, with the
//precomputed values for the fast modulo of fastmod_buckets

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/value_functors.hpp>
#include <boost/move/algo/detail/search.hpp>
#include <boost/cstdint.hpp>      //uint64_t
#include <cstddef>                //std::size_t

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace boost {
namespace intrusive {

//We only support LLP64(Win64) or LP64(most Unix) data models
#ifdef _WIN64  //In 64 bit windows sizeof(size_t) == sizeof(unsigned long long)
#  define BOOST_INTRUSIVE_SIZE_C(NUMBER) NUMBER##ULL
#  define BOOST_INTRUSIVE_64_BIT_SIZE_T 1
#else //In 32 bit windows and 32/64 bit unixes sizeof(size_t) == sizeof(unsigned long)
#  define BOOST_INTRUSIVE_SIZE_C(NUMBER) NUMBER##UL
#  define BOOST_INTRUSIVE_64_BIT_SIZE_T (((((ULONG_MAX>>16)>>16)>>16)>>15) != 0)
#endif

template<int Dummy = 0>
struct prime_list_holder
{
   private:

   //Maximum value of SizeType that std::size_t can represent
   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t max_size_type_value()
   {  return sizeof(SizeType) < sizeof(std::size_t) ? std::size_t(SizeType(-1)) : std::size_t(-1);  }

   //Converts n to std::size_t, saturating values that std::size_t can't represent
   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t saturate_to_size_t(SizeType n)
   {  return saturate_to_size_t(n, detail::bool_<(sizeof(SizeType) > sizeof(std::size_t))>());  }

   //SizeType is not wider than std::size_t, no saturation needed
   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t saturate_to_size_t(SizeType n, detail::false_)
   {  return std::size_t(n);  }

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t saturate_to_size_t(SizeType n, detail::true_)
   {  return n > SizeType(std::size_t(-1)) ? std::size_t(-1) : std::size_t(n);  }

   static const std::size_t prime_list[];
   static const std::size_t prime_list_size;

   //Biggest prime smaller or equal than n or the smallest prime
   static const std::size_t *suggested_lower_bucket_count_ptr(std::size_t n)
   {
      const std::size_t *primes     = &prime_list[0];
      const std::size_t *primes_end = primes + prime_list_size;
      std::size_t const* bound =
         boost::movelib::upper_bound(primes, primes_end, n, value_less<std::size_t>());
      bound -= std::size_t(bound != primes);
      return bound;
   }

   //Smallest prime bigger or equal than n and smaller or equal than max_n
   static const std::size_t *suggested_upper_bucket_count_ptr(std::size_t n, std::size_t max_n)
   {
      const std::size_t *primes     = &prime_list[0];
      const std::size_t *primes_end = primes + prime_list_size;
      std::size_t const* bound =
         boost::movelib::lower_bound(primes, primes_end, n, value_less<std::size_t>());
      if(bound == primes_end || *bound > max_n){
         bound = suggested_lower_bucket_count_ptr(max_n);
      }
      return bound;
   }

   public:

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static SizeType suggested_upper_bucket_count(SizeType n)
   {
      return static_cast<SizeType>
         (*suggested_upper_bucket_count_ptr(saturate_to_size_t(n), max_size_type_value<SizeType>()));
   }

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static SizeType suggested_lower_bucket_count(SizeType n)
   {  return static_cast<SizeType>(*suggested_lower_bucket_count_ptr(saturate_to_size_t(n)));   }

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t suggested_lower_bucket_count_idx(SizeType n)
   {  return static_cast<std::size_t>(suggested_lower_bucket_count_ptr(saturate_to_size_t(n)) - &prime_list[0]); }

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t suggested_upper_bucket_count_idx(SizeType n)
   {
      return static_cast<std::size_t>
         (suggested_upper_bucket_count_ptr(saturate_to_size_t(n), max_size_type_value<SizeType>()) - &prime_list[0]);
   }

   BOOST_INTRUSIVE_FORCEINLINE static std::size_t size_from_index(std::size_t n)
   {  return prime_list[std::ptrdiff_t(n)]; }

   template<std::size_t SizeIndex>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t modfunc(std::size_t hash) { return hash % SizeIndex; }

   static std::size_t(*const positions[])(std::size_t);

   #if BOOST_INTRUSIVE_64_BIT_SIZE_T
   static const uint64_t inv_sizes32[];
   static const std::size_t inv_sizes32_size;
   #endif

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t lower_size_index(SizeType n)
   {   return prime_list_holder<>::suggested_lower_bucket_count_idx(n);  }

   template <class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t upper_size_index(SizeType n)
   {   return prime_list_holder<>::suggested_upper_bucket_count_idx(n);  }

   BOOST_INTRUSIVE_FORCEINLINE static std::size_t size(std::size_t size_index)
   {   return prime_list_holder<>::size_from_index(size_index);  }

   #if BOOST_INTRUSIVE_64_BIT_SIZE_T
   // https://github.com/lemire/fastmod

   inline static uint64_t mul128_u32(uint64_t lowbits, uint32_t d)
   {
      #if defined(_MSC_VER)
         return __umulh(lowbits, d);
      #elif defined(BOOST_HAS_INT128)
         return static_cast<uint64_t>((uint128_type(lowbits) * d) >> 64);
      #else
         uint64_t r1 = (lowbits & UINT32_MAX) * d;
         uint64_t r2 = (lowbits >> 32) * d;
         r2 += r1 >> 32;
         return r2 >> 32;
      #endif
   }

   inline static uint32_t fastmod_u32(uint32_t a, uint64_t M, uint32_t d)
   {
      uint64_t lowbits = M * a;
      return (uint32_t)(mul128_u32(lowbits, d));
   }
   #endif // BOOST_INTRUSIVE_64_BIT_SIZE_T

   inline static std::size_t position(std::size_t hash,std::size_t size_index)
   {
      #if BOOST_INTRUSIVE_64_BIT_SIZE_T
         BOOST_CONSTEXPR_OR_CONST std::size_t sizes_under_32bit = sizeof(inv_sizes32)/sizeof(inv_sizes32[0]);
         if(BOOST_LIKELY(size_index < sizes_under_32bit)){
            return fastmod_u32( uint32_t(hash)+uint32_t(hash>>32)
                              , inv_sizes32[size_index]
                              , uint32_t(prime_list[size_index])  );
         }
         else{
            return positions[size_index](hash);
         }
      #else
         return positions[size_index](hash);
      #endif // BOOST_INTRUSIVE_64_BIT_SIZE_T
  }
};

template<int Dummy>
std::size_t(* const prime_list_holder<Dummy>::positions[])(std::size_t) =
{
   modfunc<BOOST_INTRUSIVE_SIZE_C(3)>,                     modfunc<BOOST_INTRUSIVE_SIZE_C(7)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(11)>,                    modfunc<BOOST_INTRUSIVE_SIZE_C(17)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(29)>,                    modfunc<BOOST_INTRUSIVE_SIZE_C(53)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(97)>,                    modfunc<BOOST_INTRUSIVE_SIZE_C(193)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(389)>,                   modfunc<BOOST_INTRUSIVE_SIZE_C(769)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1543)>,                  modfunc<BOOST_INTRUSIVE_SIZE_C(3079)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(6151)>,                  modfunc<BOOST_INTRUSIVE_SIZE_C(12289)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(24593)>,                 modfunc<BOOST_INTRUSIVE_SIZE_C(49157)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(98317)>,                 modfunc<BOOST_INTRUSIVE_SIZE_C(196613)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(393241)>,                modfunc<BOOST_INTRUSIVE_SIZE_C(786433)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1572869)>,               modfunc<BOOST_INTRUSIVE_SIZE_C(3145739)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(6291469)>,               modfunc<BOOST_INTRUSIVE_SIZE_C(12582917)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(25165843)>,              modfunc<BOOST_INTRUSIVE_SIZE_C(50331653)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(100663319)>,             modfunc<BOOST_INTRUSIVE_SIZE_C(201326611)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(402653189)>,             modfunc<BOOST_INTRUSIVE_SIZE_C(805306457)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1610612741)>,            //0-30 indexes
#if BOOST_INTRUSIVE_64_BIT_SIZE_T
   //Taken from Boost.MultiIndex code, thanks to Joaquin M. Lopez Munoz.
   modfunc<BOOST_INTRUSIVE_SIZE_C(3221225473)>,            //<- 32 bit values stop here (index 31)
   modfunc<BOOST_INTRUSIVE_SIZE_C(6442450939)>,            modfunc<BOOST_INTRUSIVE_SIZE_C(12884901893)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(25769803751)>,           modfunc<BOOST_INTRUSIVE_SIZE_C(51539607551)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(103079215111)>,          modfunc<BOOST_INTRUSIVE_SIZE_C(206158430209)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(412316860441)>,          modfunc<BOOST_INTRUSIVE_SIZE_C(824633720831)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1649267441651)>,         modfunc<BOOST_INTRUSIVE_SIZE_C(3298534883309)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(6597069766657)>,         modfunc<BOOST_INTRUSIVE_SIZE_C(13194139533299)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(26388279066623)>,        modfunc<BOOST_INTRUSIVE_SIZE_C(52776558133303)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(105553116266489)>,       modfunc<BOOST_INTRUSIVE_SIZE_C(211106232532969)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(422212465066001)>,       modfunc<BOOST_INTRUSIVE_SIZE_C(844424930131963)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1688849860263953)>,      modfunc<BOOST_INTRUSIVE_SIZE_C(3377699720527861)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(6755399441055731)>,      modfunc<BOOST_INTRUSIVE_SIZE_C(13510798882111483)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(27021597764222939)>,     modfunc<BOOST_INTRUSIVE_SIZE_C(54043195528445957)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(108086391056891903)>,    modfunc<BOOST_INTRUSIVE_SIZE_C(216172782113783843)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(432345564227567621)>,    modfunc<BOOST_INTRUSIVE_SIZE_C(864691128455135207)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(1729382256910270481)>,   modfunc<BOOST_INTRUSIVE_SIZE_C(3458764513820540933)>,
   modfunc<BOOST_INTRUSIVE_SIZE_C(6917529027641081903)>,   modfunc<BOOST_INTRUSIVE_SIZE_C(9223372036854775783)> //(index 63)
#else
   modfunc<BOOST_INTRUSIVE_SIZE_C(2147483647)>             //<- 32 bit stops here (index 31) as ptrdiff_t is signed
#endif
 };

template<int Dummy>
const std::size_t prime_list_holder<Dummy>::prime_list[] = {
   BOOST_INTRUSIVE_SIZE_C(3),                     BOOST_INTRUSIVE_SIZE_C(7),
   BOOST_INTRUSIVE_SIZE_C(11),                    BOOST_INTRUSIVE_SIZE_C(17),
   BOOST_INTRUSIVE_SIZE_C(29),                    BOOST_INTRUSIVE_SIZE_C(53),
   BOOST_INTRUSIVE_SIZE_C(97),                    BOOST_INTRUSIVE_SIZE_C(193),
   BOOST_INTRUSIVE_SIZE_C(389),                   BOOST_INTRUSIVE_SIZE_C(769),
   BOOST_INTRUSIVE_SIZE_C(1543),                  BOOST_INTRUSIVE_SIZE_C(3079),
   BOOST_INTRUSIVE_SIZE_C(6151),                  BOOST_INTRUSIVE_SIZE_C(12289),
   BOOST_INTRUSIVE_SIZE_C(24593),                 BOOST_INTRUSIVE_SIZE_C(49157),
   BOOST_INTRUSIVE_SIZE_C(98317),                 BOOST_INTRUSIVE_SIZE_C(196613),
   BOOST_INTRUSIVE_SIZE_C(393241),                BOOST_INTRUSIVE_SIZE_C(786433),
   BOOST_INTRUSIVE_SIZE_C(1572869),               BOOST_INTRUSIVE_SIZE_C(3145739),
   BOOST_INTRUSIVE_SIZE_C(6291469),               BOOST_INTRUSIVE_SIZE_C(12582917),
   BOOST_INTRUSIVE_SIZE_C(25165843),              BOOST_INTRUSIVE_SIZE_C(50331653),
   BOOST_INTRUSIVE_SIZE_C(100663319),             BOOST_INTRUSIVE_SIZE_C(201326611),
   BOOST_INTRUSIVE_SIZE_C(402653189),             BOOST_INTRUSIVE_SIZE_C(805306457),
   BOOST_INTRUSIVE_SIZE_C(1610612741),            //0-30 indexes
#if BOOST_INTRUSIVE_64_BIT_SIZE_T
   //Taken from Boost.MultiIndex code, thanks to Joaquin M. Lopez Munoz.
   BOOST_INTRUSIVE_SIZE_C(3221225473),            //<- 32 bit values stop here (index 31)
   BOOST_INTRUSIVE_SIZE_C(6442450939),            BOOST_INTRUSIVE_SIZE_C(12884901893),
   BOOST_INTRUSIVE_SIZE_C(25769803751),           BOOST_INTRUSIVE_SIZE_C(51539607551),
   BOOST_INTRUSIVE_SIZE_C(103079215111),          BOOST_INTRUSIVE_SIZE_C(206158430209),
   BOOST_INTRUSIVE_SIZE_C(412316860441),          BOOST_INTRUSIVE_SIZE_C(824633720831),
   BOOST_INTRUSIVE_SIZE_C(1649267441651),         BOOST_INTRUSIVE_SIZE_C(3298534883309),
   BOOST_INTRUSIVE_SIZE_C(6597069766657),         BOOST_INTRUSIVE_SIZE_C(13194139533299),
   BOOST_INTRUSIVE_SIZE_C(26388279066623),        BOOST_INTRUSIVE_SIZE_C(52776558133303),
   BOOST_INTRUSIVE_SIZE_C(105553116266489),       BOOST_INTRUSIVE_SIZE_C(211106232532969),
   BOOST_INTRUSIVE_SIZE_C(422212465066001),       BOOST_INTRUSIVE_SIZE_C(844424930131963),
   BOOST_INTRUSIVE_SIZE_C(1688849860263953),      BOOST_INTRUSIVE_SIZE_C(3377699720527861),
   BOOST_INTRUSIVE_SIZE_C(6755399441055731),      BOOST_INTRUSIVE_SIZE_C(13510798882111483),
   BOOST_INTRUSIVE_SIZE_C(27021597764222939),     BOOST_INTRUSIVE_SIZE_C(54043195528445957),
   BOOST_INTRUSIVE_SIZE_C(108086391056891903),    BOOST_INTRUSIVE_SIZE_C(216172782113783843),
   BOOST_INTRUSIVE_SIZE_C(432345564227567621),    BOOST_INTRUSIVE_SIZE_C(864691128455135207),
   BOOST_INTRUSIVE_SIZE_C(1729382256910270481),   BOOST_INTRUSIVE_SIZE_C(3458764513820540933),
   BOOST_INTRUSIVE_SIZE_C(6917529027641081903),   BOOST_INTRUSIVE_SIZE_C(9223372036854775783) //(index 63)
#else
   BOOST_INTRUSIVE_SIZE_C(2147483647)             //<- 32 bit stops here (index 31) as ptrdiff_t is signed
#endif
};

template<int Dummy>
const std::size_t prime_list_holder<Dummy>::prime_list_size
   = sizeof(prime_list) / sizeof(std::size_t);


#if BOOST_INTRUSIVE_64_BIT_SIZE_T

template<int Dummy>
const uint64_t prime_list_holder<Dummy>::inv_sizes32[] = {
   BOOST_INTRUSIVE_SIZE_C(6148914691236517206), //3
   BOOST_INTRUSIVE_SIZE_C(2635249153387078803), //7
   BOOST_INTRUSIVE_SIZE_C(1676976733973595602), //11
   BOOST_INTRUSIVE_SIZE_C(1085102592571150096), //17
   BOOST_INTRUSIVE_SIZE_C(636094623231363849),  //29
   BOOST_INTRUSIVE_SIZE_C(348051774975651918),  //53
   BOOST_INTRUSIVE_SIZE_C(190172619316593316),  //97
   BOOST_INTRUSIVE_SIZE_C(95578984837873325),   //193
   BOOST_INTRUSIVE_SIZE_C(47420935922132524),   //389
   BOOST_INTRUSIVE_SIZE_C(23987963684927896),   //769
   BOOST_INTRUSIVE_SIZE_C(11955116055547344),   //1543
   BOOST_INTRUSIVE_SIZE_C(5991147799191151),    //3079
   BOOST_INTRUSIVE_SIZE_C(2998982941588287),    //6151
   BOOST_INTRUSIVE_SIZE_C(1501077717772769),    //12289
   BOOST_INTRUSIVE_SIZE_C(750081082979285),     //24593
   BOOST_INTRUSIVE_SIZE_C(375261795343686),     //49157
   BOOST_INTRUSIVE_SIZE_C(187625172388393),     //98317
   BOOST_INTRUSIVE_SIZE_C(93822606204624),      //196613
   BOOST_INTRUSIVE_SIZE_C(46909513691883),      //393241
   BOOST_INTRUSIVE_SIZE_C(23456218233098),      //786433
   BOOST_INTRUSIVE_SIZE_C(11728086747027),      //1572869
   BOOST_INTRUSIVE_SIZE_C(5864041509391),       //3145739
   BOOST_INTRUSIVE_SIZE_C(2932024948977),       //6291469
   BOOST_INTRUSIVE_SIZE_C(1466014921160),       //12582917
   BOOST_INTRUSIVE_SIZE_C(733007198436),        //25165843
   BOOST_INTRUSIVE_SIZE_C(366503839517),        //50331653
   BOOST_INTRUSIVE_SIZE_C(183251896093),        //100663319
   BOOST_INTRUSIVE_SIZE_C(91625960335),         //201326611
   BOOST_INTRUSIVE_SIZE_C(45812983922),         //402653189
   BOOST_INTRUSIVE_SIZE_C(22906489714),         //805306457
   BOOST_INTRUSIVE_SIZE_C(11453246088),         //1610612741
   BOOST_INTRUSIVE_SIZE_C(5726623060)           //3221225473
};

template<int Dummy>
const std::size_t prime_list_holder<Dummy>::inv_sizes32_size
   = sizeof(inv_sizes32) / sizeof(uint64_t);

#endif // BOOST_INTRUSIVE_64_BIT_SIZE_T

struct prime_fmod_size : prime_list_holder<>
{
};


#undef BOOST_INTRUSIVE_SIZE_C
#undef BOOST_INTRUSIVE_64_BIT_SIZE_T

}  //namespace intrusive
}  //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif   //BOOST_INTRUSIVE_DETAIL_PRIME_HOLDER_HPP
