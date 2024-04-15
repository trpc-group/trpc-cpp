//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/hazptr_object.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/hazptr/hazptr_object.h"

#include "trpc/util/hazptr/hazptr_domain.h"

namespace trpc::hazptr {

void Object::PushRetired(HazptrDomain* domain) { domain->Retire(this); }

void Object::PushReclaim(HazptrDomain* domain) { domain->Reclaim(this); }

}  // namespace trpc::hazptr
