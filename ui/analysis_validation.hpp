#pragma once
#include "editing/document.hpp"
namespace drumfoundry::ui {
// Parse before mutating frames or scheduling renders. Failed imports must not
// leave a half-loaded zoom/reference or publish it into the current fit.
editing::Json ReadSavedView(const editing::Json &analysis);
void ValidateAnalysisDocument(const editing::Json &document);
} // namespace drumfoundry::ui
