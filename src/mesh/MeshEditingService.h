#pragma once
#include "ai/MeshOperation.h"

struct MeshEditResult {
    QString outputPath;
    QString error;
    double originalVolume = 0;
    double editedVolume = 0;
};

class MeshEditingService {
public:
    // Input comes exclusively from AppController's selected ViewState, never the provider.
    static MeshEditResult apply(const QString& selectedObjPath, const MeshOperation& operation);
};
