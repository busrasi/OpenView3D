#pragma once
#include <memory>
#include <QMetaType>
class Loader;
using MeshPtr = std::shared_ptr<const Loader>;
Q_DECLARE_METATYPE(MeshPtr)
