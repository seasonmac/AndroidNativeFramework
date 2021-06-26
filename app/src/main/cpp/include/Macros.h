//
// Created by season on 2021/6/24.
//

#pragma once

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))
