#include "StateBroadcaster.h"

namespace flowzone {

StateBroadcaster::StateBroadcaster() {}

StateBroadcaster::~StateBroadcaster() {}

void StateBroadcaster::setMessageCallback(MessageCallback callback) {
  juce::ScopedLock sl(lock);
  sendMessage = callback;
}

void StateBroadcaster::broadcastFullState(const AppState &state) {
  juce::ScopedLock sl(lock);
  revisionId++;

  juce::var stateVar = state.toVar();
  previousStateVar = stateVar;
  hasPreviousState = true;

  juce::DynamicObject *root = new juce::DynamicObject();
  root->setProperty("type", "STATE_FULL");
  root->setProperty("revisionId", revisionId);
  root->setProperty("data", stateVar);

  if (sendMessage) {
    juce::String jsonString = juce::JSON::toString(juce::var(root));
    sendMessage(jsonString);
  }
}

void StateBroadcaster::broadcastStateUpdate(const AppState &state) {
  juce::ScopedLock sl(lock);
  
  juce::var newStateVar = state.toVar();

  // If no previous state, send full snapshot
  if (!hasPreviousState) {
    revisionId++;
    previousStateVar = newStateVar;
    hasPreviousState = true;

    juce::DynamicObject *root = new juce::DynamicObject();
    root->setProperty("type", "STATE_FULL");
    root->setProperty("revisionId", revisionId);
    root->setProperty("data", newStateVar);

    if (sendMessage) {
      juce::String jsonString = juce::JSON::toString(juce::var(root));
      sendMessage(jsonString);
    }
    return;
  }

  // Generate JSON Patch operations
  juce::Array<juce::var> patchOps = generateJsonPatch(previousStateVar, newStateVar, "");

  // If no changes, don't send anything
  if (patchOps.isEmpty()) {
    return;
  }

  // Calculate patch size
  size_t patchSize = calculatePatchSize(patchOps);
  
  // Decide: patch or snapshot?
  // Spec: >20 ops or >4KB → send snapshot
  const int MAX_PATCH_OPS = 20;
  const size_t MAX_PATCH_BYTES = 4096;

  if (patchOps.size() > MAX_PATCH_OPS || patchSize > MAX_PATCH_BYTES) {
    // Send full snapshot
    revisionId++;
    previousStateVar = newStateVar;

    juce::DynamicObject *root = new juce::DynamicObject();
    root->setProperty("type", "STATE_FULL");
    root->setProperty("revisionId", revisionId);
    root->setProperty("data", newStateVar);

    if (sendMessage) {
      juce::String jsonString = juce::JSON::toString(juce::var(root));
      sendMessage(jsonString);
    }
  } else {
    // Send patch
    revisionId++;
    previousStateVar = newStateVar;

    juce::DynamicObject *root = new juce::DynamicObject();
    root->setProperty("type", "STATE_PATCH");
    root->setProperty("revisionId", revisionId);
    root->setProperty("ops", patchOps);

    if (sendMessage) {
      juce::String jsonString = juce::JSON::toString(juce::var(root));
      sendMessage(jsonString);
    }
  }
}

int64_t StateBroadcaster::getRevisionId() const { return revisionId; }

juce::Array<juce::var> StateBroadcaster::generateJsonPatch(
    const juce::var &oldVal, const juce::var &newVal,
    const juce::String &path) {
  juce::Array<juce::var> ops;

  // Both are objects - recurse into properties
  if (oldVal.isObject() && newVal.isObject()) {
    auto *oldObj = oldVal.getDynamicObject();
    auto *newObj = newVal.getDynamicObject();

    if (!oldObj || !newObj)
      return ops;

    // Check all properties in new object
    auto newProps = newObj->getProperties();
    for (int i = 0; i < newProps.size(); ++i) {
      juce::String propName = newProps.getName(i).toString();
      juce::var newPropVal = newProps.getValueAt(i);
      juce::String propPath = path + "/" + propName;

      if (oldObj->hasProperty(propName)) {
        juce::var oldPropVal = oldObj->getProperty(propName);
        // Recurse to compare property values
        auto propOps = generateJsonPatch(oldPropVal, newPropVal, propPath);
        ops.addArray(propOps);
      } else {
        // Property added
        juce::DynamicObject *op = new juce::DynamicObject();
        op->setProperty("op", "add");
        op->setProperty("path", propPath);
        op->setProperty("value", newPropVal);
        ops.add(juce::var(op));
      }
    }

    // Check for removed properties
    auto oldProps = oldObj->getProperties();
    for (int i = 0; i < oldProps.size(); ++i) {
      juce::String propName = oldProps.getName(i).toString();
      if (!newObj->hasProperty(propName)) {
        juce::String propPath = path + "/" + propName;
        juce::DynamicObject *op = new juce::DynamicObject();
        op->setProperty("op", "remove");
        op->setProperty("path", propPath);
        ops.add(juce::var(op));
      }
    }
  }
  // Both are arrays - compare elements
  else if (oldVal.isArray() && newVal.isArray()) {
    auto *oldArr = oldVal.getArray();
    auto *newArr = newVal.getArray();

    if (!oldArr || !newArr)
      return ops;

    int oldSize = oldArr->size();
    int newSize = newArr->size();
    int minSize = juce::jmin(oldSize, newSize);

    // Compare existing elements
    for (int i = 0; i < minSize; ++i) {
      juce::String elemPath = path + "/" + juce::String(i);
      auto elemOps = generateJsonPatch(oldArr->getUnchecked(i),
                                       newArr->getUnchecked(i), elemPath);
      ops.addArray(elemOps);
    }

    // Handle added elements
    for (int i = minSize; i < newSize; ++i) {
      juce::String elemPath = path + "/-"; // RFC 6902: "-" means append
      juce::DynamicObject *op = new juce::DynamicObject();
      op->setProperty("op", "add");
      op->setProperty("path", elemPath);
      op->setProperty("value", newArr->getUnchecked(i));
      ops.add(juce::var(op));
    }

    // Handle removed elements (remove from end to maintain indices)
    for (int i = oldSize - 1; i >= newSize; --i) {
      juce::String elemPath = path + "/" + juce::String(i);
      juce::DynamicObject *op = new juce::DynamicObject();
      op->setProperty("op", "remove");
      op->setProperty("path", elemPath);
      ops.add(juce::var(op));
    }
  }
  // Primitive values - check if different
  else {
    // Compare values
    bool isDifferent = false;

    if (oldVal.isString() && newVal.isString()) {
      isDifferent = (oldVal.toString() != newVal.toString());
    } else if (oldVal.isDouble() && newVal.isDouble()) {
      isDifferent = (static_cast<double>(oldVal) != static_cast<double>(newVal));
    } else if (oldVal.isInt() && newVal.isInt()) {
      isDifferent = (static_cast<int>(oldVal) != static_cast<int>(newVal));
    } else if (oldVal.isInt64() && newVal.isInt64()) {
      isDifferent = (static_cast<int64_t>(oldVal) != static_cast<int64_t>(newVal));
    } else if (oldVal.isBool() && newVal.isBool()) {
      isDifferent = (static_cast<bool>(oldVal) != static_cast<bool>(newVal));
    } else {
      // Type mismatch or other types
      isDifferent = true;
    }

    if (isDifferent && !path.isEmpty()) {
      juce::DynamicObject *op = new juce::DynamicObject();
      op->setProperty("op", "replace");
      op->setProperty("path", path);
      op->setProperty("value", newVal);
      ops.add(juce::var(op));
    }
  }

  return ops;
}

size_t StateBroadcaster::calculatePatchSize(
    const juce::Array<juce::var> &ops) const {
  // Serialize the ops array to JSON and measure byte length
  juce::String jsonStr = juce::JSON::toString(juce::var(ops));
  return jsonStr.toUTF8().sizeInBytes() - 1; // -1 for null terminator
}

} // namespace flowzone
