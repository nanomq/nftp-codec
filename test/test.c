// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include "test.h"

int
main()
{
	test_hash();
	test_file();
	test_vector();
	test_iovs();
	test_iter();
	test_codec();
	test_proto();
}
