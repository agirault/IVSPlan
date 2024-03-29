// MRML includes
#include "vtkEventBroker.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLCameraNode.h"
#include "vtkMRMLCameraPathNode.h"
#include "vtkMRMLPointSplineNode.h"
#include "vtkMRMLCameraPathStorageNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>


//------------------------------------------------------------------------------
// timeComp

//----------------------------------------------------------------------------


//------------------------------------------------------------------------------
// vtkMRMLCameraPathNode::vtkInternal

//------------------------------------------------------------------------------
class vtkMRMLCameraPathNode::vtkInternal
{
public:
  vtkInternal(vtkMRMLCameraPathNode* external);
  ~vtkInternal();

  KeyFrameVector KeyFrames;
  vtkSmartPointer<vtkMRMLPointSplineNode> Positions;
  vtkSmartPointer<vtkMRMLPointSplineNode> FocalPoints;
  vtkSmartPointer<vtkMRMLPointSplineNode> ViewUps;

  vtkMRMLCameraPathNode* External;
};

//------------------------------------------------------------------------------
vtkMRMLCameraPathNode::vtkInternal::vtkInternal(
    vtkMRMLCameraPathNode* external):External(external)
{
  this->Positions = vtkSmartPointer<vtkMRMLPointSplineNode>::New();
  this->FocalPoints = vtkSmartPointer<vtkMRMLPointSplineNode>::New();
  this->ViewUps = vtkSmartPointer<vtkMRMLPointSplineNode>::New();
}

//------------------------------------------------------------------------------
vtkMRMLCameraPathNode::vtkInternal::~vtkInternal()
{
    // TODO : delete cameras in list ?
}

//------------------------------------------------------------------------------
// vtkMRMLCameraPathNode

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLCameraPathNode)

//----------------------------------------------------------------------------
vtkMRMLCameraPathNode::vtkMRMLCameraPathNode()
{
  this->Internal = new vtkInternal(this);
  this->HideFromEditors = 0;
}

//----------------------------------------------------------------------------
vtkMRMLCameraPathNode::~vtkMRMLCameraPathNode()
{
  this->RemoveKeyFrames();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::Copy(vtkMRMLNode *anode)
{
  vtkMRMLCameraPathNode *node =
      vtkMRMLCameraPathNode::SafeDownCast(anode);
  if (!node)
    {
    vtkErrorMacro("Node empty");
    return;
    }

  int disabledModify = this->StartModify();

  this->Superclass::Copy(anode);

  KeyFrameVector KeyFrames;
  vtkNew<vtkMRMLPointSplineNode> Positions;
  vtkNew<vtkMRMLPointSplineNode> FocalPoints;
  vtkNew<vtkMRMLPointSplineNode> ViewUps;

  KeyFrames = node->GetKeyFrames();
  Positions->Copy(node->GetPositionSplines());
  FocalPoints->Copy(node->GetFocalPointSplines());
  ViewUps->Copy(node->GetViewUpSplines());

  this->SetKeyFrames(KeyFrames);
  this->SetPointSplines(Positions.GetPointer(),
                        FocalPoints.GetPointer(),
                        ViewUps.GetPointer());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  double numPts = this->GetNumberOfKeyFrames();
  os << indent << "NumberOfKeyFrames: " << numPts << "\n";
  os << indent << "MinimumT: " << this->GetMinimumT() << "\n";
  os << indent << "MaximumT: " << this->GetMaximumT() << "\n";

  for( vtkIdType i = 0; i < numPts; ++i)
    {
      KeyFrame keyFrame = this->GetKeyFrame(i);
      os << indent << "KeyFrame " << i << ":\n";
      os << indent.GetNextIndent() << "Time: " << keyFrame.Time << "\n";
      os << indent.GetNextIndent() << "Camera: " << keyFrame.Camera->GetID() << "\n";
    }
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLCameraPathNode::CreateDefaultStorageNode()
{
  return vtkMRMLStorageNode::SafeDownCast(vtkMRMLCameraPathStorageNode::New());
}

//----------------------------------------------------------------------------
vtkIdType vtkMRMLCameraPathNode::GetNumberOfKeyFrames()
{
  return this->GetKeyFrames().size();
}

//----------------------------------------------------------------------------
double vtkMRMLCameraPathNode::GetMinimumT()
{
  if(this->GetKeyFrames().empty())
    {
    vtkWarningMacro("No key frames");
    return 0;
    }
  return this->GetKeyFrames().front().Time;
}

//----------------------------------------------------------------------------
double vtkMRMLCameraPathNode::GetMaximumT()
{
  if(this->GetKeyFrames().empty())
    {
    vtkWarningMacro("No key frames");
    return 0;
    }
  return this->GetKeyFrames().back().Time;
}

//----------------------------------------------------------------------------
KeyFrameVector vtkMRMLCameraPathNode::GetKeyFrames()
{
  return this->Internal->KeyFrames;
}

//----------------------------------------------------------------------------
KeyFrame
vtkMRMLCameraPathNode::GetKeyFrame(vtkIdType index)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return KeyFrame();
    }
  return this->GetKeyFrames().at(index);
}

//----------------------------------------------------------------------------
double vtkMRMLCameraPathNode::GetKeyFrameTime(vtkIdType index)
{
  return this->GetKeyFrame(index).Time;
}

//---------------------------------------------------------------------------
vtkMRMLCameraNode *vtkMRMLCameraPathNode::GetKeyFrameCamera(vtkIdType index)
{
  return this->GetKeyFrame(index).Camera;
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetKeyFramePosition(vtkIdType index,
                                                double position[3])
{
  if(position)
    {
    this->GetKeyFrameCamera(index)->GetPosition(position);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetKeyFrameFocalPoint(vtkIdType index,
                                                  double focalPoint[3])
{
  if(focalPoint)
    {
    this->GetKeyFrameCamera(index)->GetFocalPoint(focalPoint);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetKeyFrameViewUp(vtkIdType index,
                                              double viewUp[3])
{
  if(viewUp)
    {
    this->GetKeyFrameCamera(index)->GetViewUp(viewUp);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrames(KeyFrameVector keyFrames)
{
  this->Internal->KeyFrames = keyFrames;
  this->SortKeyFrames();

  this->Modified();
  // TODO : Update splines
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrame(vtkIdType index, KeyFrame keyFrame)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }

  if( this->GetKeyFrame(index) == keyFrame)
    {
    vtkWarningMacro("Key frame identical : no effect");
    return;
    }
  vtkIdType otherIndex = this->KeyFrameIndexAt(keyFrame.Time);
  if (otherIndex != -1)
    {
    std::cerr << "A keyframe already exists for t = " << time << std::endl
              << "Keyframe ID : " << otherIndex << std::endl
              << std::endl;
    return;
    }

  KeyFrame oldKeyFrame = this->GetKeyFrame(index);

  this->GetPositionSplines()->RemovePoint(oldKeyFrame.Time);
  this->GetFocalPointSplines()->RemovePoint(oldKeyFrame.Time);
  this->GetViewUpSplines()->RemovePoint(oldKeyFrame.Time);

  this->Internal->KeyFrames.at(index) = keyFrame;
  this->SortKeyFrames();

  this->GetPositionSplines()->AddPoint(keyFrame.Time, keyFrame.Camera->GetPosition());
  this->GetFocalPointSplines()->AddPoint(keyFrame.Time, keyFrame.Camera->GetFocalPoint());
  this->GetViewUpSplines()->AddPoint(keyFrame.Time, keyFrame.Camera->GetViewUp());
  this->CreatePath();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrameTime(vtkIdType index, double time)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }
  if( this->GetKeyFrameTime(index) == time)
    {
    vtkWarningMacro("Time identical : no effect");
    return;
    }
  vtkIdType otherIndex = this->KeyFrameIndexAt(time);
  if (otherIndex != -1)
    {
    std::cerr << "A keyframe already exists for t = " << time << std::endl
              << "Keyframe ID : " << otherIndex << std::endl
              << std::endl;
    return;
    }

  KeyFrame oldKeyFrame = this->GetKeyFrame(index);

  this->GetPositionSplines()->RemovePoint(oldKeyFrame.Time);
  this->GetFocalPointSplines()->RemovePoint(oldKeyFrame.Time);
  this->GetViewUpSplines()->RemovePoint(oldKeyFrame.Time);

  this->Internal->KeyFrames.at(index).Time = time;
  this->SortKeyFrames();
\
  this->GetPositionSplines()->AddPoint(time, oldKeyFrame.Camera->GetPosition());
  this->GetFocalPointSplines()->AddPoint(time, oldKeyFrame.Camera->GetFocalPoint());
  this->GetViewUpSplines()->AddPoint(time, oldKeyFrame.Camera->GetViewUp());
  this->CreatePath();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrameCamera(vtkIdType index,
                                              vtkMRMLCameraNode* camera)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }

// XXX If the camera is modified, the pointSplines are not automatically
// updated, so we need to do a SetKeyFrameCamera
/*
  if( this->GetKeyFrameCamera(index) == camera)
    {
    vtkWarningMacro("Camera identical : no effect");
    return;
    }
*/
  this->Internal->KeyFrames.at(index).Camera = camera;

  double time = this->GetKeyFrame(index).Time;
  this->GetPositionSplines()->AddPoint(time, camera->GetPosition());
  this->GetFocalPointSplines()->AddPoint(time, camera->GetFocalPoint());
  this->GetViewUpSplines()->AddPoint(time, camera->GetViewUp());
  this->CreatePath();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFramePosition(vtkIdType index,
                                                double position[3])
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }
  double testPos[3];
  this->GetKeyFramePosition(index, testPos);
  if( testPos[0] == position[0] &&
          testPos[1] == position[1] &&
          testPos[2] == position[2])
    {
    vtkWarningMacro("Position identical : no effect");
    return;
    }

  this->Internal->KeyFrames.at(index).Camera->SetPosition(position);

  double time = this->GetKeyFrame(index).Time;
  this->GetPositionSplines()->AddPoint(time, position);
  this->CreatePath();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrameFocalPoint(vtkIdType index,
                                                  double focalPoint[3])
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }
  double testFoc[3];
  this->GetKeyFrameFocalPoint(index, testFoc);
  if( testFoc[0] == focalPoint[0] &&
          testFoc[1] == focalPoint[1] &&
          testFoc[2] == focalPoint[2])
    {
    vtkWarningMacro("Focal point identical : no effect");
    return;
    }

  this->Internal->KeyFrames.at(index).Camera->SetFocalPoint(focalPoint);

  double time = this->GetKeyFrame(index).Time;
  this->GetFocalPointSplines()->AddPoint(time, focalPoint);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetKeyFrameViewUp(vtkIdType index,
                                              double viewUp[3])
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }
  double testView[3];
  this->GetKeyFrameViewUp(index, testView);
  if( testView[0] == viewUp[0] &&
          testView[1] == viewUp[1] &&
          testView[2] == viewUp[2])
    {
    vtkWarningMacro("View up identical : no effect");
    return;
    }

  this->Internal->KeyFrames.at(index).Camera->SetPosition(viewUp);

  double time = this->GetKeyFrame(index).Time;
  this->GetViewUpSplines()->AddPoint(time, viewUp);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::AddKeyFrame(KeyFrame keyFrame)
{
  double t = keyFrame.Time;
  vtkIdType index = this->KeyFrameIndexAt(t);
  if (index != -1)
    {
    std::cerr << "A keyframe already exists for t = " << t << std::endl
              << "Keyframe ID : " << index << std::endl
              << "To update it, use the methods 'SetKeyFrame(index)' instead."
              << std::endl;
    return;
    }
  this->Internal->KeyFrames.push_back(keyFrame);
  this->SortKeyFrames();

  this->GetPositionSplines()->AddPoint(t, keyFrame.Camera->GetPosition());
  this->GetFocalPointSplines()->AddPoint(t, keyFrame.Camera->GetFocalPoint());
  this->GetViewUpSplines()->AddPoint(t, keyFrame.Camera->GetViewUp());
  this->CreatePath();

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::AddKeyFrame(double t,
                                        vtkMRMLCameraNode* camera)
{
  KeyFrame keyFrame(camera, t);
  this->AddKeyFrame(keyFrame);
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::AddKeyFrame(double t,
                                        double position[3],
                                        double focalPoint[3],
                                        double viewUp[3])
{
  vtkNew<vtkMRMLCameraNode> camera;
  camera->SetPosition(position);
  camera->SetFocalPoint(focalPoint);
  camera->SetViewUp(viewUp);
  this->AddKeyFrame(t, camera.GetPointer());
}

//---------------------------------------------------------------------------
vtkIdType vtkMRMLCameraPathNode::KeyFrameIndexAt(double t)
{
  KeyFrameVector::iterator it = find_if(this->Internal->KeyFrames.begin(),
                                       this->Internal->KeyFrames.end(),
                                       timeEqual(t));
  if (it != this->Internal->KeyFrames.end())
    {
    vtkIdType index = it - this->Internal->KeyFrames.begin();
    return index;
    }
  return -1;
}
//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::RemoveKeyFrames()
{
  // Update splines
  this->GetPositionSplines()->RemoveAllPoints();
  this->GetFocalPointSplines()->RemoveAllPoints();
  this->GetViewUpSplines()->RemoveAllPoints();
  this->CreatePath();

  // Remove cameras
  for (vtkIdType i = 0; i < this->GetNumberOfKeyFrames(); ++i)
    {
    this->RemoveCameraFromScene(i);
    }

  // Remove keyframes
  this->Internal->KeyFrames.clear();

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::RemoveKeyFrame(vtkIdType index)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }

  // Update splines
  double t = this->GetKeyFrame(index).Time;
  this->GetPositionSplines()->RemovePoint(t);
  this->GetFocalPointSplines()->RemovePoint(t);
  this->GetViewUpSplines()->RemovePoint(t);
  this->CreatePath();

  // Remove camera
  this->RemoveCameraFromScene(index);

  // Remove keyframe
  this->Internal->KeyFrames.erase(
              this->Internal->KeyFrames.begin() + index);

  this->Modified();
}


//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::RemoveCameraFromScene(vtkIdType index)
{
  if( index < 0 || index >= this->GetNumberOfKeyFrames() )
    {
    vtkErrorMacro("No key frame at this index");
    return;
    }

  vtkMRMLScene* scene = this->GetKeyFrameCamera(index)->GetScene();
  if(scene)
    {
    scene->RemoveNode(this->GetKeyFrameCamera(index));
    }
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SortKeyFrames()
{
  if( this->GetNumberOfKeyFrames() < 2)
    {
    return;
    }
  std::sort(this->Internal->KeyFrames.begin(),
            this->Internal->KeyFrames.end());
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::CreatePath()
{
  this->GetPositionSplines()->UpdatePolyData(30);
}

//----------------------------------------------------------------------------
vtkMRMLPointSplineNode* vtkMRMLCameraPathNode::GetPositionSplines()
{
  return this->Internal->Positions;
}

//----------------------------------------------------------------------------
vtkMRMLPointSplineNode* vtkMRMLCameraPathNode::GetFocalPointSplines()
{
  return this->Internal->FocalPoints;
}

//----------------------------------------------------------------------------
vtkMRMLPointSplineNode* vtkMRMLCameraPathNode::GetViewUpSplines()
{
  return this->Internal->ViewUps;
}

//----------------------------------------------------------------------------
void vtkMRMLCameraPathNode::SetPointSplines(vtkMRMLPointSplineNode* positions,
                                            vtkMRMLPointSplineNode* focalPoints,
                                            vtkMRMLPointSplineNode* viewUps)
{
  this->Internal->Positions = positions;
  this->Internal->FocalPoints = focalPoints;
  this->Internal->ViewUps = viewUps;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetCameraAt(double t,
                      vtkMRMLCameraNode* camera)
{
  double position[3], focalPoint[3], viewUp[3];
  this->GetPositionAt(t, position);
  this->GetFocalPointAt(t, focalPoint);
  this->GetViewUpAt(t, viewUp);

  camera->SetPosition(position);
  camera->SetFocalPoint(focalPoint);
  camera->SetViewUp(viewUp);
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetPositionAt(double t, double position[3])
{
  t = this->ClampTime(t);
  this->GetPositionSplines()->Evaluate(t, position);
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetFocalPointAt(double t, double focalPoint[3])
{
  t = this->ClampTime(t);
  this->GetFocalPointSplines()->Evaluate(t, focalPoint);
}

//---------------------------------------------------------------------------
void vtkMRMLCameraPathNode::GetViewUpAt(double t, double viewUp[3])
{
  this->ClampTime(t);
  this->GetViewUpSplines()->Evaluate(t, viewUp);
}

//---------------------------------------------------------------------------
double vtkMRMLCameraPathNode::ClampTime(double t)
{
  if ( t < this->GetMinimumT() )
    {
    t = this->GetMinimumT();
    }
  else if ( t > this->GetMaximumT() )
    {
    t = this->GetMaximumT();
    }
  return t;
}
