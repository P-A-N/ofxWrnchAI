#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "engine.hpp"

class ofxWrnchAI
{
public:

	struct Person
	{
		struct Joint { glm::vec2 p; string name; };
		struct Bone { Joint from, to; };

		struct Pose { std::vector<Joint> joints; std::vector<Bone> bones; };
		struct Body : public Pose {};
		struct Hand : public Pose { int left_or_right; /*left = 0, right = 1*/ };
		struct Face : public Pose {};
		Body body;
		std::vector<Hand> hands;
		Face face;
	};

	ofxWrnchAI(
		const string license_str, 
		const string pose_def = "j25", const string hand_def = "hand21", const string face_def = "wrFace20",
		const string model_path = "../wrModels")
	{
		auto licenseCheckCode = wrnch::LicenseCheckString(license_str);
		if (licenseCheckCode != wrReturnCode_OK)
		{
			ofLogError("ofxWrnchAI") << "Error: " << wrReturnCode_Translate(licenseCheckCode);
			ofSystemAlertDialog(wrReturnCode_Translate(licenseCheckCode));
			ofExit();
		}

		std::vector<const char*> definitionList(wrnch::JointDefinitionRegistry::GetNumDefinitions());
		wrnch::JointDefinitionRegistry::GetAvailableDefinitions(definitionList.data());

		for (unsigned int i = 0; i < definitionList.size(); i++)
		{
			wrnch::JointDefinition definition = wrnch::JointDefinitionRegistry::Get(definitionList[i]);
			definition.PrintJointDefinition();
		}


		auto modelsDirectory = ofToDataPath(model_path, true);
		ofLogNotice("ofxWrnchAI") << "Initializing networks from " << modelsDirectory << " ...";

		wrnch::PoseParams params;
		params.SetEnableTracking(false);
		params.SetPreferredNetWidth2d(456);
		params.SetPreferredNetHeight2d(256);

		pose_joint_def = wrnch::JointDefinitionRegistry::Get(pose_def.c_str());
		hand_joint_def = wrnch::JointDefinitionRegistry::Get(hand_def.c_str());
		face_joint_def = wrnch::JointDefinitionRegistry::Get(face_def.c_str());

		ofLogNotice("ofxWrnchAI") << "pose_joint_def = " << pose_joint_def.GetName();
		ofLogNotice("ofxWrnchAI") << "hand_joint_def = " << hand_joint_def.GetName();
		ofLogNotice("ofxWrnchAI") << "face_joint_def = " << face_joint_def.GetName();

		try
		{
			int gpuIndex = 0;
			poseEstimator.Initialize(modelsDirectory.c_str(), gpuIndex, params, pose_joint_def);
		}
		catch (const std::exception& e)
		{
			ofLogError("ofxWrnchAI Initialize") << "Error: " << e.what();
			ofExit();
		}
		catch (...)
		{
			ofLogError("ofxWrnchAI Initialize") << "Unknown error occurred";
			ofExit();
		}

		try
		{
			poseEstimator.InitializeHands2D(modelsDirectory.c_str());
		}
		catch (const std::exception& e)
		{
			ofLogError("ofxWrnchAI InitializeHands2D") << "Error: " << e.what();
			ofExit();
		}
		catch (...)
		{
			ofLogError("ofxWrnchAI InitializeHands2D") << "Unknown error occurred";
			ofExit();
		}

		try
		{
			poseEstimator.InitializeHead(modelsDirectory.c_str());
		}
		catch (const std::exception& e)
		{
			ofLogError("ofxWrnchAI InitializeHead") << "Error: " << e.what();
			ofExit();
		}
		catch (...)
		{
			ofLogError("ofxWrnchAI InitializeHead") << "Unknown error occurred";
			ofExit();
		}

		poseOptions.SetEstimateMask(false);
		poseOptions.SetEstimateAllHandBoxes(true);
		poseOptions.SetEstimateHands(true);
		poseOptions.SetMainPersonId(wrnch_MAIN_ID_ALL);
		poseOptions.SetEstimateHead(true);
		poseOptions.SetEstimatePoseFace(true);

		num_pose_joint = poseEstimator.GetHuman2DOutputFormat().GetNumJoints();
		num_pose_bone = pose_joint_def.GetNumBones();
		pose_bone_pairs = std::vector<unsigned int>(num_pose_bone * 2);
		pose_joint_def.GetBonePairs(pose_bone_pairs.data());
		
		hand_bone_pairs = std::vector<unsigned int>(hand_joint_def.GetNumBones() * 2);
		hand_joint_def.GetBonePairs(&hand_bone_pairs[0]);
		num_hand_joint = poseEstimator.GetHandOutputFormat().GetNumJoints();
		num_hand_bone = hand_joint_def.GetNumBones();

		face_bone_pairs = std::vector<unsigned int>(face_joint_def.GetNumBones() * 2);
		face_joint_def.GetBonePairs(face_bone_pairs.data());
		num_face_joint = face_joint_def.GetNumJoints();
		num_face_bone = face_joint_def.GetNumBones();
	}

	void update(ofPixels& px)
	{
		auto frame = ofxCv::toCv(px);
		try
		{
			poseEstimator.ProcessFrame(frame.data, frame.cols, frame.rows, poseOptions);
		}
		catch (const std::exception& e)
		{
			ofLogError("ofxWrnchAI") << "Error: " << e.what();
			return;
		}
		catch (...)
		{
			ofLogError("ofxWrnchAI") << "Unknown error occurred";
			return;
		}
	}

	void debug_draw(const int w, const int h)
	{
		for (wrnch::PoseEstimator::Humans2dIter it = poseEstimator.Humans2dBegin();
			it < poseEstimator.Humans2dEnd();
			it++)
		{
			wrnch::Pose2dView pose = *it;
			draw_point(pose.GetJoints(), num_pose_joint, ofColor::red, 5, w, h);
			draw_line(pose.GetJoints(), &pose_bone_pairs[0], num_pose_bone, ofColor::yellow, 2, w, h);
		}

		draw_hands(
			poseEstimator.LeftHandsBegin(), poseEstimator.LeftHandsEnd(),
			&hand_bone_pairs[0], num_hand_joint, num_hand_bone, w, h);

		draw_hands(
			poseEstimator.RightHandsBegin(), poseEstimator.RightHandsEnd(),
			&hand_bone_pairs[0], num_hand_joint, num_hand_bone, w, h);

		for (wrnch::PoseEstimator::PoseFaceIter it = poseEstimator.PoseFacesBegin();
			it < poseEstimator.PoseFacesEnd();
			it++)
		{
			wrnch::PoseFaceView face = *it;
			draw_point(face.GetLandmarks(), num_face_joint, ofColor::blue, 2, w, h);
			draw_line(face.GetLandmarks(), &face_bone_pairs[0], num_face_bone, ofColor::green, 1, w, h);
		}
	}

	std::vector<Person> get_structured_data(const int w, const int h)
	{
		auto feed_joint = [](
			Person::Pose& pose_data, 
			const int num_pose_joint, const float* points, const vector<string> joint_names,
			const int w, const int h) -> void
		{
			for (auto i = 0; i < num_pose_joint; i++)
			{
				Person::Joint joint;
				joint.p = glm::vec2(points[2 * i] * w, points[2 * i + 1] * h);
				joint.name = joint_names[i];
				pose_data.joints.push_back(joint);
			}
		};
		auto feed_bone = [](
			Person::Pose& pose_data, 
			const int num_pose_bone, const std::vector<unsigned int> pose_bone_pairs) -> void
		{
			for (auto i = 0; i < num_pose_bone; i++)
			{
				Person::Bone bone;
				auto idx1 = pose_bone_pairs[2 * i];
				auto idx2 = pose_bone_pairs[2 * i + 1];
				bone.from = pose_data.joints[idx1];
				bone.to = pose_data.joints[idx2];
				pose_data.bones.push_back(bone);
			}
		};

		std::vector<Person> persons;
		persons.resize(poseEstimator.GetNumHumans2D());

		auto person_idx = 0;
		for (wrnch::PoseEstimator::Humans2dIter it = poseEstimator.Humans2dBegin();
			it < poseEstimator.Humans2dEnd();
			it++)
		{
			wrnch::Pose2dView pose = *it;
			auto joint_names = pose_joint_def.GetJointNames();
			Person::Body body;
			feed_joint(body, num_pose_joint, pose.GetJoints(), joint_names, w, h);
			feed_bone(body, num_pose_bone, pose_bone_pairs);
			persons[person_idx].body = body;
			person_idx++;
		}
		
		person_idx = 0;
		for (wrnch::PoseEstimator::PoseHandsIter it = poseEstimator.LeftHandsBegin(); 
			it < poseEstimator.LeftHandsEnd(); 
			it++)
		{
			wrnch::Pose2dView pose = *it;
			auto joint_names = hand_joint_def.GetJointNames();
			Person::Hand hand; hand.left_or_right = 0;
			feed_joint(hand, num_hand_joint, pose.GetJoints(), joint_names, w, h);
			feed_bone(hand, num_hand_bone, hand_bone_pairs);
			persons[person_idx].hands.push_back(hand);
			person_idx++;
		}

		person_idx = 0;
		for (wrnch::PoseEstimator::PoseHandsIter it = poseEstimator.RightHandsBegin();
			it < poseEstimator.RightHandsEnd();
			it++)
		{
			wrnch::Pose2dView pose = *it;
			auto joint_names = hand_joint_def.GetJointNames();
			Person::Hand hand; hand.left_or_right = 1;
			feed_joint(hand, num_hand_joint, pose.GetJoints(), joint_names, w, h);
			feed_bone(hand, num_hand_bone, hand_bone_pairs);
			persons[person_idx].hands.push_back(hand);
			person_idx++;
		}

		person_idx = 0;
		for (wrnch::PoseEstimator::PoseFaceIter it = poseEstimator.PoseFacesBegin();
			it < poseEstimator.PoseFacesEnd();
			it++)
		{
			wrnch::PoseFaceView face = *it;
			auto joint_names = face_joint_def.GetJointNames();
			Person::Face face_data;
			feed_joint(face_data, num_face_joint, face.GetLandmarks(), joint_names, w, h);
			feed_bone(face_data, num_face_bone, face_bone_pairs);
			persons[person_idx].face = face_data;
			person_idx++;
		}

		return persons;
	}

private:

	void draw_hands(
		wrnch::PoseEstimator::PoseHandsIter handsBegin,
		wrnch::PoseEstimator::PoseHandsIter handsEnd,
		const unsigned int* bonePairs,
		const unsigned int numHandJoints,
		const unsigned int numBones,
		const int w, const int h)
	{
		for (wrnch::PoseEstimator::PoseHandsIter it = handsBegin; it < handsEnd; ++it)
		{
			wrnch::Pose2dView pose = *it;
			draw_point(pose.GetJoints(), numHandJoints, ofColor::cyan, 2, w, h);
			draw_line(pose.GetJoints(), &bonePairs[0], numBones, ofColor::magenta, 1, w, h);
		}
	}

	void draw_point(
		const float* points, unsigned int numPoints, const ofColor color, float pointSize,
		const int w, const int h)
	{
		ofPushStyle();
		ofSetColor(color);
		for (unsigned i = 0; i < numPoints; i++)
		{
			int x = points[2 * i] * w;
			int y = points[2 * i + 1] * h;
			if (x >= 0 && y >= 0)
				ofDrawCircle(x, y, pointSize);
		}
		ofPopStyle();
	}

	void draw_line(const float* points, const unsigned int* pairs, unsigned int numPairs,
		const ofColor color, int thickness,
		const int w, const int h)
	{
		ofPushStyle();
		ofSetLineWidth(thickness);
		ofSetColor(color);
		for (unsigned int i = 0; i < numPairs; i++)
		{
			unsigned const idx1 = pairs[2 * i];
			unsigned const idx2 = pairs[2 * i + 1];
			int x1 = points[idx1 * 2] * w;
			int y1 = points[idx1 * 2 + 1] * h;
			int x2 = points[idx2 * 2] * w;
			int y2 = points[idx2 * 2 + 1] * h;
			if (x1 >= 0 && x2 >= 0 && y1 >= 0 && y2 >= 0)
				ofDrawLine(glm::vec2(x1, y1), glm::vec2(x2, y2));
		}
		ofSetLineWidth(1);
		ofPopStyle();
	}

	wrnch::PoseEstimator poseEstimator;
	wrnch::PoseEstimatorOptions poseOptions;

	wrnch::JointDefinition pose_joint_def;
	std::vector<unsigned int> pose_bone_pairs;
	int num_pose_joint, num_pose_bone;

	wrnch::JointDefinition hand_joint_def;
	std::vector<unsigned int> hand_bone_pairs;
	int num_hand_joint, num_hand_bone;

	wrnch::JointDefinition face_joint_def;
	std::vector<unsigned int> face_bone_pairs;
	int num_face_joint, num_face_bone;
};