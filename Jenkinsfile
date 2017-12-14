env.CACHE_BASEDIR = env.WORKSPACE

pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                slackSend color: "#439FE0", message: slack_build_message("started")
                ansiColor('xterm') {
                    sh 'cmake -Bbuild -Hcorvis -DMKLROOT=False -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=$GIT_COMMIT'
                    sh 'cmake --build build -- -j'
                }
            }
        }
        stage('Test') {
            steps {
                sh 'cmake --build build --target test'
            }
        }
        stage('Build slam_client') {
            steps {
                sh '''#!/bin/bash
                    source ./corvis/src/movidius/mvenv
                    cd ./corvis/src/movidius/device
                    make -j
                '''
            }
        }
        stage('Run benchmark') {
            steps {
                withCredentials([string(credentialsId: 'slackBenchmarkToken', variable: 'SLACK_BENCHMARK_TOKEN')]) {
                    sh 'build/measure --qvga --benchmark $JENKINS_HOME/benchmark_data/new_test_suite/ --benchmark-output benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt'
                    sh 'sed -ne /^Length/,//p benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt                           > benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                    sh 'curl -F file=@benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt -F channels=#slam_build -F token=$SLACK_BENCHMARK_TOKEN https://slack.com/api/files.upload'
                    archiveArtifacts artifacts: "benchmark-*-$BRANCH_NAME-${GIT_COMMIT}.txt"
                }
            }
        }
    }
    post {
        always {
            deleteDir()
        }
        success {
            slackSend color: "good", message: slack_build_message("succeeded")
        }
        failure {
            slackSend color: "#FF0000", message: slack_build_message("failed")
        }
    }
}

def slack_build_message(status) {
    "<$BUILD_URL|Build> of " + (env.CHANGE_ID
        ? "<${env.CHANGE_URL}|#${env.CHANGE_ID} ${env.CHANGE_TITLE}> for ${env.CHANGE_TARGET} by ${env.CHANGE_AUTHOR}"
        : "<${env.GIT_URL.replaceAll(/\.git$/,'')}/tree/${env.GIT_BRANCH}|${env.BRANCH_NAME}>") +
    " $status (<${env.BUILD_URL}/artifact/benchmark-details-$BRANCH_NAME-${env.GIT_COMMIT}.txt|benchmark>)"
}
