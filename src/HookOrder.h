#pragma once

#include <QVector>
#include <algorithm>
#include <functional>

// Snapshot hook ids in priority order (highest first, registration order
// preserved for ties) before any callback runs. Callbacks may hook/unhook,
// which mutates the live vector; the snapshot stays valid.
template <typename Hook>
QVector<int> orderedMatchingHookIds(const QVector<Hook> &hooks,
                                    const std::function<bool(const Hook &)> &pred) {
    struct Item {
        int id;
        int priority;
        int order;
    };
    QVector<Item> items;
    items.reserve(hooks.size());
    for (int i = 0; i < hooks.size(); ++i) {
        if (pred(hooks.at(i)))
            items.append(Item{hooks.at(i).id, hooks.at(i).priority, i});
    }
    std::stable_sort(items.begin(), items.end(), [](const Item &a, const Item &b) {
        return a.priority > b.priority;
    });
    QVector<int> ids;
    ids.reserve(items.size());
    for (const Item &it : items)
        ids.append(it.id);
    return ids;
}

// Pointer is invalidated by any later append/remove on `hooks`.
// Copy the fields you need before invoking a callback.
template <typename Hook>
const Hook *findHookById(const QVector<Hook> &hooks, int id) {
    for (const Hook &hook : hooks) {
        if (hook.id == id)
            return &hook;
    }
    return nullptr;
}
