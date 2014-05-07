### Intrusive Lists with Bounded Pointers

#### Introduction

The existing intrusive list implementation targets two specific use cases with respect to the pointers allowed in the data structures: plain pointers, and smart pointers. This work is motivated by a third, distinct, use case.

Suppose that in order to save memory, we want to replace 8-byte pointers by 1-byte indices into an array. More specifically, we are interested in placing all `value` objects the program will ever deal with into an array, and we want to build intrusive lists containing some of these objects, where list pointers between them are replaced by array indices. We henceforth refer to these indices as *bounded pointers*.

#### Observations on Implementing the Use Case

Part of our goal is to integrate this use case as seamlessly as possible with the existing code base. With this in mind, here are some primary observations:

- We need to set aside 1 of the 256 possible values to represent `NULL`. So we are left with 255 valid bounded pointers.

- The existing code base makes a distinction between the notions of `node` and `value`. One should be able to efficiently convert `node` pointers from and to `value` pointers. For simplicity, in our case, we will forego this type of customization, and assume we are only interested in the case where `node == value`.

- The existing code base makes a distinction between non-constant pointers (`node_ptr`) and constant pointers (`const_node_ptr`). Having a single type implement both results in conflicting function declarations. For this reason, we need to implement access control for bounded pointers, along with the usual semantics: implicit conversion to const, explicit conversion to nonconst.

- To preserve generality, we will assume that the raw-to-bounded pointer conversion is inefficient. So, given the address of a `value` object, we assume there is no easy way to obtain a bounded pointer to it. Perhaps the motivation for this restriction is unclear in the context of a plain array: given the address of an object in an array, one can find its index by simply subtracting the address of the first element in the array. However, we are also interested in situations where the objects are stored in a dynamic deque, for which raw-to-bounded conversion is inefficient.

- The existing code base has methods for manipulating `value` references, e.g. `void push_back(reference)`. Because of the issue explained above, in our use case we cannot use raw references for such methods. Instead, we use a "reference proxy" scheme. Specifically, a bounded pointer's `operator *` produces a bounded reference object, and this object has the same internal representation as a pointer (an array index), but different behaviour: its `operator &` produces back the bounded pointer, and its `operator value&()` allows for implicit conversion to a real reference. We also implement usual access control by having const and nonconst bounded references.

#### Observations on the Existing Intrusive List Implementation

While the existing code base is not unfriendly towards proxy references, there are some issues that arise:

- In the existing intrusive list implementation, the header node of the list is part of the list struct. Thus, even though intrusive data structures in general come with the promise of allowing external memory management of the type held by the list, this promise is violated by the header node in the case when `node == value`. Specifically, in our use case, constructing a `value` object outside of the array containing all other `value` objects makes it impossible to refer to that object using a bounded pointer. While it is technically possible to work around this by instantiating the list with `node` being a `node` "holder" object, this entails having `node != pointer_traits<node_ptr>::element_type`, which can in turn lead to other problems. This motivated us in adding the option to specify a node allocator during list instantiation and construction.

- If the list struct contains only a header node pointer (instead of the header node itself), it is generally impossible to retrieve the address of the list given the header node pointer alone. This is what made methods based on `priv_container_from_end_iterator()` work, and as a result these had to be disabled in the case of an external header.


#### Changes to the Intrusive List Implementation

##### Option to specify a node allocator

We added the option to specify a node allocator type at list instantiation. This is done using the new `node_allocator_type< Allocator >` list option. By default, `Allocator = std::allocator<void>`. Furthermore, the allocator may be stateful, and a specific instance may be specified at list construction time. Internally, the new `allocator` object is used much in way of the existing `value_traits` object. If an allocator is not specified at list construction, one is default constructed from its `Allocator` type. The allocator object is moved during move construction.

The `Allocator` type must always define inner types `value_type` and `pointer`. If `Allocator::value_type == void`, the allocator is not used. Otherwise, it is required (asserted) that `Allocator::value_type == node` and `Allocator::pointer == node_ptr`. The only `Allocator` methods currently used are `pointer allocator.allocate(1)` and `void allocator.deallocate(p, 1)`.

##### New list static bool constants

- `has_node_allocator == true` iff `Allocator::value_type == node`.

- `external_header == true` iff `has_node_allocator == true`. In other words, whenever a non-void node allocator type is specified, the header will be external. Conceptually, one could imagine this being a separate option, but for now they are the same.

- `has_container_from_iterator == true` iff `external_header == false` AND `node == pointer_traits<node_ptr>::element_type`. When this is false, the methods implementing container-from-iterator will be unavailable.

- `has_value_allocator == true` iff `has_node_allocator == true` AND `node == value_type`. If a non-void allocator is given, the list has the option to use a trivial cloner and disposer based on this allocator.

#### New list typedefs

- `node_allocator_type`: template parameter `Node_Allocator`, default: `std::allocator< void >`.

- `value_allocator_type`: if `has_value_allocator == true`, this equals `node_allocator_type`; else `std::allocator< void >`. One can imagine a situation where this is set independently of `node_allocator_type`, but that is not implemented yet.

- `header_traits`: specialization of the `detail::header_holder` struct; holds a header node or header node pointer.

- `header_from_node_ptr_functor`: specialization of the `detail::header_from_node_ptr` functor; implements header node pointer to list transformation, when `has_container_from_iterator == true`.

#### New behaviour

- The list struct will hold an `Allocator` object. If these are stateful, this will add to the list footprint, much in the same way a stateful `value_traits` object does.

- When `external_header == true`, the list struct will contain a header `node_ptr` instead of a header `node`. The actual storage is allocated with the given node allocator, and deallocated upon destruction of the list.

- When `has_container_from_iterator == false`, methods relying on `priv_container_from_end_iterator()` will be unavailable. Using them will trigger a static assert.

- When `has_value_allocator == true`, two new methods have been added: `void clear_and_dispose()` and `void clone_from(const list_impl &)`. The difference is that they do not require a disposer (first) or a cloner and disposer (second) compared to the regular ones. When used, a trivial cloner and disposer will be used based on the list's existing node allocator.

#### New utility structs in `detail/utilities.hpp`

- `detail::header_holder` holds either a header node or a header node pointer.

- `detail::header_from_node_ptr` is a functor implementing the conversion from head node pointer to list struct, when `has_container_from_iterator == false`.

- `detail::cloner_from_allocator` implements a basic cloner using an allocator: allocate value, then copy construct it in place.

- `detail::disposer_from_allocator` implements a basic disposer using an allocator: call value destructor, then deallocate it.

#### Changes to the Intrusive List Test Suite

We extended the existing test suite to also test for non-void node allocators (we use `std::allocator< node >`) used with plain pointers. For these settings, the same const/nonconst size options and hook options are tested, as in the case of void allocators. In addition to that, we test the new bounded pointer use case.

##### Changes to `test/itestvalue.hpp`

- For the functors `even_odd` and `is_even`: We changed the template parametrization to the full value type (they were previously parametrized by parameters of the value type, but not using that information).

- For the functors `even_odd` and `is_even`: We also replaced the use of `operator .` with `operator &` plus `operator ->`. As explained later, for the new use case, the values held in the test array will be proxy references to the tested `value_type`. C++ provides no way to overload `operator .`, thus `v.x` applied to a proxy reference will not invoke its implicit conversion to `value_type&`. This is avoided by `(&v)->x`, where both operators can be overloaded. In all existing tests, the test arrays hold real values, which are completely unaffected by this change.

- We added a global method `swap_nodes()` that implements the swapping of nodes. The overload in `test/itestvalue.hpp` captures all calls with values specialized from `testvalue`, and simply delegates it to the `swap_nodes` member function, which was used before. This extra method is necessary because in the new use case, `bptr_value` (the new value type) cannot derive its bounded address. So the swap must be implemented at the level of proxy references.

##### Changes to `test/list_test.cpp`

While there are quite a few changes seen in a plain `diff`, they were motivated by only several concerns.

- As seen in `main()`, all existing list declarations that were tested before are still tested now, with a void node allocator (the default option). In addition to that, those declarations with plain pointers are also tested with the standard non-void node allocator. The new use case is implemented by values specialized from `bptr_value`. These are tested with constant and non-constant size options, and always with non-trivial allocators.

- We changed the parametrization of the `test_list` struct from `Value_Traits` to `List_Type, Value_Container`. The reason for the first is that previously, lists were declared using options packed by `Value_Traits` (specifically, the constant size option), and we needed to add more options (the allocator type). Also, every specific test declared its own list, making it unclear they were all testing the same list type. The reason for the `Value_Container` parametrization is that in the new use case the test vector cannot pack objects of type `value_type`, but instead it has to pack proxy references. Thus, for values specialized from `testvalue`, `Value_Container == std::vector< value_type >`, whereas for values specialized from `bptr_value`, `Value_Container == std::vector< reference >` (almost, see below).

- `test_remove_unique` contains a copy of the test array. Applied to `std::vector< reference >`, this would copy references, not values. We were able to salvage this by defining and using instead a `Bounded_Reference_Cont< value_type >` which derives from `std::vector< reference >`, but implements a deep copy in its copy constructor.

- Expressions of the type `&values[0] + 2` were previously used as iterators. This does not work in the new use case, when `&values[0]` is a bounded pointer. Instead, we replace these by `values.begin() + 2` which works as expected in both use cases.

- We removed an unnecessary `const_cast<const value_type &>`. Even when written as `const_cast<const_reference>`, this does work with proxy references because `const_cast` expects only pointers or raw references.

- We replaced the call to the `swap_nodes` method of a value by a call to a global `swap_nodes`. Values specialized from `testvalue` and `bptr_value` have different overloads for this method.

- `test_container_from_end` cannot be run on lists where `has_container_from_iterator == false`. We provide two functors based on this condition, one containing the real test, the other containing a log message only.

- For `test_clone`, when `has_value_allocator == true`, the test will not provide cloner and disposer objects, thus testing the new functionality where these are derived from the existing node allocator.

- `make_and_test_list` gathers list parameters and dispatches the call to `test_list`. This is only used for values specialized from `testvalue`, where the allocators are either void or standard.

- `test_main_template_bptr` tests the new values specialized from `bptr_value`. The allocator is non-trivial in this case, specifically, it is `Bouned_Allocator< value_type >`. All values are allocated using this (stateless) allocator. In the end, we check there are no leaks.

##### A sample bounded pointer implementation

The new use case is implemented by the bounded pointers in `test/bounded_pointer.hpp` and the associated values in `test/bptr_value.hpp`.
